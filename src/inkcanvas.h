#pragma once

#include "stroke.h"

#include <QImage>
#include <QQuickPaintedItem>

// Pen surface tuned for e-ink: black-only strokes, incremental painting of new
// segments (small dirty rects -> fast partial refresh) and no animations.
// Input arrives from QML (PointHandler) so pressure and eraser tip are known.
class InkCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY strokesChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY strokesChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)

public:
    explicit InkCanvas(QQuickItem *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(const QString &path);
    qreal penWidth() const { return m_penWidth; }
    void setPenWidth(qreal w);
    bool isEmpty() const { return m_strokes.isEmpty(); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool isModified() const { return m_modified; }

    Q_INVOKABLE void beginStroke(qreal x, qreal y, qreal pressure);
    Q_INVOKABLE void extendStroke(qreal x, qreal y, qreal pressure);
    Q_INVOKABLE void endStroke();
    Q_INVOKABLE void eraseAt(qreal x, qreal y, qreal radius = 12);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void undo();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void reload();

    const QVector<Stroke> &strokes() const { return m_strokes; }

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void penWidthChanged();
    void strokesChanged();
    void modifiedChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void rebuildBuffer();
    void pushUndo();
    void setModified(bool m);
    QPointF norm(qreal x, qreal y) const;

    QString m_source;
    qreal m_penWidth = 2.5;
    QVector<Stroke> m_strokes;
    QVector<QVector<Stroke>> m_undo;
    bool m_drawing = false;
    bool m_modified = false;
    QImage m_buffer;
};
