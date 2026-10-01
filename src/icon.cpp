#include "icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

Icon::Icon(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setImplicitSize(24, 24);
}

void Icon::setName(const QString &n)
{
    if (n == m_name)
        return;
    m_name = n;
    emit changed();
    update();
}

void Icon::setColor(const QColor &c)
{
    if (c == m_color)
        return;
    m_color = c;
    emit changed();
    update();
}

QStringList Icon::names()
{
    return {"menu", "today", "project", "tag", "worklog", "focus", "sync", "settings", "plus", "check",
            "play", "pause", "pen", "pen-fine", "pen-thick", "eraser", "undo", "redo", "close", "calendar",
            "more", "chevron-left", "chevron-right", "chevron-down", "trash", "subtask", "keyboard",
            "template", "export", "circle", "done-list", "priority", "estimate", "back", "update", "inbox"};
}

// All drawing happens on a 24x24 grid (like common icon sets), scaled to the item.
void Icon::paint(QPainter *p)
{
    const qreal s = qMin(width(), height()) / 24.0;
    p->save();
    p->translate((width() - 24 * s) / 2, (height() - 24 * s) / 2);
    p->scale(s, s);
    p->setRenderHint(QPainter::Antialiasing);
    QPen pen(m_color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    const QString &n = m_name;
    auto fill = [&] { p->setBrush(m_color); };

    if (n == QLatin1String("menu")) {
        p->drawLine(QPointF(4, 7), QPointF(20, 7));
        p->drawLine(QPointF(4, 12), QPointF(20, 12));
        p->drawLine(QPointF(4, 17), QPointF(20, 17));
    } else if (n == QLatin1String("today")) {
        p->drawEllipse(QPointF(12, 12), 4, 4);
        for (int i = 0; i < 8; ++i) {
            const qreal a = i * M_PI / 4;
            p->drawLine(QPointF(12 + 6.5 * qCos(a), 12 + 6.5 * qSin(a)), QPointF(12 + 9 * qCos(a), 12 + 9 * qSin(a)));
        }
    } else if (n == QLatin1String("project")) {
        QPainterPath path;
        path.moveTo(3, 6.5);
        path.lineTo(3, 18.5);
        path.lineTo(21, 18.5);
        path.lineTo(21, 8.5);
        path.lineTo(11.5, 8.5);
        path.lineTo(9.5, 5.5);
        path.lineTo(3.5, 5.5);
        path.closeSubpath();
        p->drawPath(path);
    } else if (n == QLatin1String("inbox")) {
        p->drawRoundedRect(QRectF(3.5, 4.5, 17, 15), 2, 2);
        QPainterPath path;
        path.moveTo(3.5, 13);
        path.lineTo(8.5, 13);
        path.lineTo(10, 15.5);
        path.lineTo(14, 15.5);
        path.lineTo(15.5, 13);
        path.lineTo(20.5, 13);
        p->drawPath(path);
    } else if (n == QLatin1String("tag")) {
        QPainterPath path;
        path.moveTo(3.5, 3.5);
        path.lineTo(11.5, 3.5);
        path.lineTo(20.5, 12.5);
        path.lineTo(12.5, 20.5);
        path.lineTo(3.5, 11.5);
        path.closeSubpath();
        p->drawPath(path);
        p->drawEllipse(QPointF(8, 8), 1.3, 1.3);
    } else if (n == QLatin1String("worklog")) {
        p->drawRoundedRect(QRectF(4.5, 3.5, 15, 17), 1.5, 1.5);
        p->drawLine(QPointF(8, 8.5), QPointF(16, 8.5));
        p->drawLine(QPointF(8, 12.5), QPointF(16, 12.5));
        p->drawLine(QPointF(8, 16.5), QPointF(12.5, 16.5));
    } else if (n == QLatin1String("focus") || n == QLatin1String("estimate")) {
        p->drawEllipse(QPointF(12, 13), 7.5, 7.5);
        p->drawLine(QPointF(12, 13), QPointF(12, 9));
        if (n == QLatin1String("focus")) {
            p->drawLine(QPointF(10, 3), QPointF(14, 3));
            p->drawLine(QPointF(12, 3), QPointF(12, 5.5));
        } else {
            p->drawLine(QPointF(12, 13), QPointF(15, 15));
        }
    } else if (n == QLatin1String("sync") || n == QLatin1String("update")) {
        p->drawArc(QRectF(4.5, 4.5, 15, 15), 30 * 16, 150 * 16);
        p->drawArc(QRectF(4.5, 4.5, 15, 15), 210 * 16, 150 * 16);
        p->drawLine(QPointF(18.5, 8.2), QPointF(18.8, 4.5));
        p->drawLine(QPointF(18.5, 8.2), QPointF(14.8, 8));
        p->drawLine(QPointF(5.5, 15.8), QPointF(5.2, 19.5));
        p->drawLine(QPointF(5.5, 15.8), QPointF(9.2, 16));
    } else if (n == QLatin1String("settings")) {
        // cog: 8 flat teeth around a ring
        QPainterPath cog;
        const int teeth = 8;
        for (int i = 0; i < teeth * 4; ++i) {
            const qreal a = (i - 0.5) * M_PI * 2 / (teeth * 4);
            const qreal r = (i % 4 == 0 || i % 4 == 1) ? 9.2 : 7.2;
            const QPointF pt(12 + r * qCos(a), 12 + r * qSin(a));
            if (i == 0)
                cog.moveTo(pt);
            else
                cog.lineTo(pt);
        }
        cog.closeSubpath();
        p->drawPath(cog);
        p->drawEllipse(QPointF(12, 12), 3, 3);
    } else if (n == QLatin1String("plus")) {
        p->drawLine(QPointF(12, 5), QPointF(12, 19));
        p->drawLine(QPointF(5, 12), QPointF(19, 12));
    } else if (n == QLatin1String("check")) {
        p->drawLine(QPointF(5, 12.5), QPointF(10, 17.5));
        p->drawLine(QPointF(10, 17.5), QPointF(19.5, 7));
    } else if (n == QLatin1String("done-list")) {
        p->drawRect(QRectF(4, 5, 5, 5));
        p->drawLine(QPointF(5, 7.5), QPointF(6.5, 9));
        p->drawLine(QPointF(6.5, 9), QPointF(8.5, 6));
        p->drawRect(QRectF(4, 14, 5, 5));
        p->drawLine(QPointF(12, 7.5), QPointF(20, 7.5));
        p->drawLine(QPointF(12, 16.5), QPointF(20, 16.5));
    } else if (n == QLatin1String("play")) {
        fill();
        QPainterPath path;
        path.moveTo(8.5, 6);
        path.lineTo(18, 12);
        path.lineTo(8.5, 18);
        path.closeSubpath();
        p->drawPath(path);
    } else if (n == QLatin1String("pause")) {
        fill();
        p->drawRect(QRectF(7.5, 6, 2.5, 12));
        p->drawRect(QRectF(14, 6, 2.5, 12));
    } else if (n.startsWith(QLatin1String("pen"))) {
        QPainterPath path;
        path.moveTo(15.5, 4.5);
        path.lineTo(19.5, 8.5);
        path.lineTo(9, 19);
        path.lineTo(4.5, 19.5);
        path.lineTo(5, 15);
        path.closeSubpath();
        p->drawPath(path);
        p->drawLine(QPointF(13.5, 6.5), QPointF(17.5, 10.5));
        if (n != QLatin1String("pen")) {
            QPen line(m_color, n == QLatin1String("pen-fine") ? 1.0 : 3.2, Qt::SolidLine, Qt::RoundCap);
            p->setPen(line);
            p->drawLine(QPointF(12, 22), QPointF(21, 22));
        }
    } else if (n == QLatin1String("eraser")) {
        QPainterPath path;
        path.moveTo(9, 20);
        path.lineTo(4, 15);
        path.lineTo(14, 5);
        path.lineTo(20.5, 11.5);
        path.lineTo(12, 20);
        path.closeSubpath();
        p->drawPath(path);
        p->drawLine(QPointF(8.5, 10.5), QPointF(15, 17));
        p->drawLine(QPointF(9, 20), QPointF(20, 20));
    } else if (n == QLatin1String("undo") || n == QLatin1String("redo")) {
        if (n == QLatin1String("redo")) {
            p->translate(24, 0);
            p->scale(-1, 1);
        }
        QPainterPath path;
        path.moveTo(5, 10);
        path.lineTo(15, 10);
        path.cubicTo(22, 10, 22, 19, 15, 19);
        path.lineTo(9, 19);
        p->drawPath(path);
        p->drawLine(QPointF(5, 10), QPointF(9, 6));
        p->drawLine(QPointF(5, 10), QPointF(9, 14));
    } else if (n == QLatin1String("close")) {
        p->drawLine(QPointF(6, 6), QPointF(18, 18));
        p->drawLine(QPointF(18, 6), QPointF(6, 18));
    } else if (n == QLatin1String("calendar")) {
        p->drawRoundedRect(QRectF(3.5, 5, 17, 15), 1.5, 1.5);
        p->drawLine(QPointF(3.5, 9.5), QPointF(20.5, 9.5));
        p->drawLine(QPointF(8, 3), QPointF(8, 6.5));
        p->drawLine(QPointF(16, 3), QPointF(16, 6.5));
        fill();
        p->drawEllipse(QPointF(8.5, 14), 0.8, 0.8);
        p->drawEllipse(QPointF(12, 14), 0.8, 0.8);
        p->drawEllipse(QPointF(15.5, 14), 0.8, 0.8);
    } else if (n == QLatin1String("more")) {
        fill();
        p->drawEllipse(QPointF(5.5, 12), 1.3, 1.3);
        p->drawEllipse(QPointF(12, 12), 1.3, 1.3);
        p->drawEllipse(QPointF(18.5, 12), 1.3, 1.3);
    } else if (n == QLatin1String("chevron-left") || n == QLatin1String("back")) {
        p->drawLine(QPointF(15, 5), QPointF(8, 12));
        p->drawLine(QPointF(8, 12), QPointF(15, 19));
    } else if (n == QLatin1String("chevron-right")) {
        p->drawLine(QPointF(9, 5), QPointF(16, 12));
        p->drawLine(QPointF(16, 12), QPointF(9, 19));
    } else if (n == QLatin1String("chevron-down")) {
        p->drawLine(QPointF(5, 9), QPointF(12, 16));
        p->drawLine(QPointF(12, 16), QPointF(19, 9));
    } else if (n == QLatin1String("trash")) {
        p->drawLine(QPointF(4, 6.5), QPointF(20, 6.5));
        p->drawLine(QPointF(9.5, 6.5), QPointF(10, 3.5));
        p->drawLine(QPointF(10, 3.5), QPointF(14, 3.5));
        p->drawLine(QPointF(14, 3.5), QPointF(14.5, 6.5));
        QPainterPath path;
        path.moveTo(6, 6.5);
        path.lineTo(7, 20.5);
        path.lineTo(17, 20.5);
        path.lineTo(18, 6.5);
        p->drawPath(path);
        p->drawLine(QPointF(10, 10), QPointF(10, 17));
        p->drawLine(QPointF(14, 10), QPointF(14, 17));
    } else if (n == QLatin1String("subtask")) {
        p->drawRect(QRectF(3.5, 3.5, 6, 6));
        p->drawRect(QRectF(12.5, 14.5, 6, 6));
        p->drawLine(QPointF(6.5, 9.5), QPointF(6.5, 17.5));
        p->drawLine(QPointF(6.5, 17.5), QPointF(12.5, 17.5));
    } else if (n == QLatin1String("keyboard")) {
        p->drawRoundedRect(QRectF(2.5, 6, 19, 12), 1.5, 1.5);
        fill();
        for (int r = 0; r < 2; ++r)
            for (int c = 0; c < 5; ++c)
                p->drawRect(QRectF(5 + c * 3.1, 8.5 + r * 3, 1.3, 1.3));
        p->drawRect(QRectF(8, 14.8, 8, 1.2));
    } else if (n == QLatin1String("template")) {
        p->drawRect(QRectF(5, 3.5, 14, 17));
        p->drawLine(QPointF(7.5, 8), QPointF(16.5, 8));
        p->drawLine(QPointF(7.5, 12), QPointF(16.5, 12));
        p->drawLine(QPointF(7.5, 16), QPointF(16.5, 16));
    } else if (n == QLatin1String("export")) {
        p->drawLine(QPointF(12, 3.5), QPointF(12, 14.5));
        p->drawLine(QPointF(12, 3.5), QPointF(8, 7.5));
        p->drawLine(QPointF(12, 3.5), QPointF(16, 7.5));
        QPainterPath path;
        path.moveTo(5, 11);
        path.lineTo(5, 20.5);
        path.lineTo(19, 20.5);
        path.lineTo(19, 11);
        p->drawPath(path);
    } else if (n == QLatin1String("priority")) {
        p->drawLine(QPointF(6, 3.5), QPointF(6, 21));
        QPainterPath path;
        path.moveTo(6, 4.5);
        path.lineTo(19, 4.5);
        path.lineTo(15.5, 9);
        path.lineTo(19, 13.5);
        path.lineTo(6, 13.5);
        p->drawPath(path);
    } else if (n == QLatin1String("circle")) {
        p->drawEllipse(QPointF(12, 12), 8.5, 8.5);
    }
    p->restore();
}
