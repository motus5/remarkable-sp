#include "inkcanvas.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QSaveFile>

static constexpr int kMaxUndo = 50;

InkCanvas::InkCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setOpaquePainting(false);
}

void InkCanvas::setSource(const QString &path)
{
    if (path == m_source)
        return;
    if (m_modified)
        save();
    m_source = path;
    emit sourceChanged();
    reload();
}

void InkCanvas::setPenWidth(qreal w)
{
    if (qFuzzyCompare(w, m_penWidth))
        return;
    m_penWidth = w;
    emit penWidthChanged();
}

QPointF InkCanvas::norm(qreal x, qreal y) const
{
    const qreal w = width() > 0 ? width() : 1;
    return QPointF(x / w, y / w);
}

void InkCanvas::pushUndo()
{
    m_redo.clear();
    m_undo.append(m_strokes);
    if (m_undo.size() > kMaxUndo)
        m_undo.removeFirst();
}

void InkCanvas::setModified(bool m)
{
    if (m == m_modified)
        return;
    m_modified = m;
    emit modifiedChanged();
}

void InkCanvas::beginStroke(qreal x, qreal y, qreal pressure)
{
    pushUndo();
    Stroke s;
    s.width = float(m_penWidth / (width() > 0 ? width() : 1));
    const QPointF p = norm(x, y);
    s.points.append({float(p.x()), float(p.y()), float(pressure)});
    m_strokes.append(s);
    m_drawing = true;

    if (!m_buffer.isNull()) {
        QPainter painter(&m_buffer);
        painter.setRenderHint(QPainter::Antialiasing);
        ink::paintStroke(painter, m_strokes.last(), width());
    }
    update(QRect(int(x - 10), int(y - 10), 20, 20));
    emit strokesChanged();
}

void InkCanvas::extendStroke(qreal x, qreal y, qreal pressure)
{
    if (!m_drawing || m_strokes.isEmpty())
        return;
    Stroke &s = m_strokes.last();
    const QPointF p = norm(x, y);
    const InkPoint &last = s.points.last();
    // Drop samples closer than ~0.5px, the digitizer reports far more than we need.
    if (qAbs(last.x - p.x()) * width() < 0.5 && qAbs(last.y - p.y()) * width() < 0.5)
        return;
    s.points.append({float(p.x()), float(p.y()), float(pressure)});

    if (!m_buffer.isNull()) {
        QPainter painter(&m_buffer);
        painter.setRenderHint(QPainter::Antialiasing);
        ink::paintStroke(painter, s, width(), s.points.size() - 1);
    }
    const qreal lx = last.x * width(), ly = last.y * width();
    const int pad = int(m_penWidth * 2 + 4);
    update(QRectF(QPointF(qMin(lx, x), qMin(ly, y)), QPointF(qMax(lx, x), qMax(ly, y)))
               .toAlignedRect()
               .adjusted(-pad, -pad, pad, pad));
}

void InkCanvas::endStroke()
{
    if (!m_drawing)
        return;
    m_drawing = false;
    setModified(true);
    emit strokesChanged();
}

void InkCanvas::eraseAt(qreal x, qreal y, qreal radius)
{
    const QPointF p = norm(x, y);
    const float r = float(radius / (width() > 0 ? width() : 1));
    bool pushed = false;
    for (int i = m_strokes.size() - 1; i >= 0; --i) {
        if (m_strokes[i].hitTest(p, r)) {
            if (!pushed) {
                pushUndo();
                pushed = true;
            }
            m_strokes.removeAt(i);
        }
    }
    if (pushed) {
        setModified(true);
        rebuildBuffer();
        emit strokesChanged();
    }
}

void InkCanvas::clear()
{
    if (m_strokes.isEmpty())
        return;
    pushUndo();
    m_strokes.clear();
    setModified(true);
    rebuildBuffer();
    emit strokesChanged();
}

void InkCanvas::undo()
{
    if (m_undo.isEmpty())
        return;
    m_redo.append(m_strokes);
    m_strokes = m_undo.takeLast();
    setModified(true);
    rebuildBuffer();
    emit strokesChanged();
}

void InkCanvas::redo()
{
    if (m_redo.isEmpty())
        return;
    m_undo.append(m_strokes);
    m_strokes = m_redo.takeLast();
    setModified(true);
    rebuildBuffer();
    emit strokesChanged();
}

bool InkCanvas::save()
{
    if (m_source.isEmpty())
        return false;
    if (m_strokes.isEmpty()) {
        QFile::remove(m_source);
        setModified(false);
        return true;
    }
    QDir().mkpath(QFileInfo(m_source).absolutePath());
    QSaveFile f(m_source);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(ink::serialize(m_strokes));
    const bool ok = f.commit();
    if (ok)
        setModified(false);
    return ok;
}

void InkCanvas::reload()
{
    m_strokes.clear();
    m_undo.clear();
    m_redo.clear();
    QFile f(m_source);
    if (!m_source.isEmpty() && f.open(QIODevice::ReadOnly))
        m_strokes = ink::deserialize(f.readAll());
    setModified(false);
    rebuildBuffer();
    emit strokesChanged();
}

void InkCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        rebuildBuffer();
}

void InkCanvas::rebuildBuffer()
{
    const QSize sz = QSizeF(width(), height()).toSize();
    if (sz.isEmpty()) {
        m_buffer = QImage();
        return;
    }
    m_buffer = QImage(sz, QImage::Format_ARGB32_Premultiplied);
    m_buffer.fill(Qt::transparent);
    QPainter painter(&m_buffer);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const Stroke &s : std::as_const(m_strokes))
        ink::paintStroke(painter, s, width());
    update();
}

void InkCanvas::setPaperTemplate(const QString &t)
{
    if (t == m_template)
        return;
    m_template = t;
    emit paperChanged();
    update();
}

void InkCanvas::setLineSpacing(qreal s)
{
    if (qFuzzyCompare(s, m_spacing))
        return;
    m_spacing = s;
    emit paperChanged();
    update();
}

void InkCanvas::paint(QPainter *painter)
{
    if (m_spacing > 4 && m_template != QLatin1String("blank")) {
        // Light grey like the reMarkable templates; only the dirty area is drawn.
        const QRectF clip = painter->clipBoundingRect().isEmpty() ? boundingRect() : painter->clipBoundingRect();
        const qreal sp = m_spacing;
        const qreal x0 = sp * 0.5;
        const qreal x1 = width() - sp * 0.5;
        if (m_template == QLatin1String("dots")) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0x90, 0x90, 0x90));
            const qreal r = qMax(1.0, sp / 22);
            for (qreal y = sp; y < height(); y += sp) {
                if (y < clip.top() - sp || y > clip.bottom() + sp)
                    continue;
                for (qreal x = x0; x <= x1; x += sp)
                    painter->drawEllipse(QPointF(x, y), r, r);
            }
        } else {
            painter->setPen(QPen(QColor(0xc4, 0xc4, 0xc4), 1));
            for (qreal y = sp; y < height(); y += sp)
                if (y >= clip.top() - 1 && y <= clip.bottom() + 1)
                    painter->drawLine(QPointF(m_template == QLatin1String("grid") ? 0 : x0, y),
                                      QPointF(m_template == QLatin1String("grid") ? width() : x1, y));
            if (m_template == QLatin1String("grid"))
                for (qreal x = sp / 2; x < width(); x += sp)
                    painter->drawLine(QPointF(x, clip.top()), QPointF(x, clip.bottom()));
        }
    }

    if (!m_buffer.isNull())
        painter->drawImage(0, 0, m_buffer);
}
