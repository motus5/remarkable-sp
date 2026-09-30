#pragma once

#include <QJsonArray>
#include <QPointF>
#include <QVector>

class QPainter;

// One pen sample. Coordinates are normalised by the canvas width so ink keeps
// its shape when it is rendered at a different size (e.g. list thumbnails).
struct InkPoint {
    float x = 0;
    float y = 0;
    float pressure = 1;
};

struct Stroke {
    QVector<InkPoint> points;
    float width = 1; // base width, also normalised by canvas width

    bool hitTest(const QPointF &p, float radius) const;
    QJsonArray toJson() const;
    static Stroke fromJson(const QJsonArray &a);
};

namespace ink {
// Draws the segment between points[from-1] and points[from..] (or the full
// stroke when from == 0) scaled to a canvas of the given pixel width.
void paintStroke(QPainter &p, const Stroke &s, qreal scale, int from = 0);

QByteArray serialize(const QVector<Stroke> &strokes);
QVector<Stroke> deserialize(const QByteArray &data);
}
