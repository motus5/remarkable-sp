#include "stroke.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLineF>
#include <QPainter>
#include <QPen>

bool Stroke::hitTest(const QPointF &p, float radius) const
{
    for (int i = 0; i < points.size(); ++i) {
        const QPointF a(points[i].x, points[i].y);
        if (QLineF(a, p).length() <= radius)
            return true;
        if (i > 0) {
            // distance from p to segment (prev, a)
            const QPointF b(points[i - 1].x, points[i - 1].y);
            const QPointF d = a - b;
            const qreal len2 = QPointF::dotProduct(d, d);
            if (len2 > 0) {
                const qreal t = qBound(0.0, QPointF::dotProduct(p - b, d) / len2, 1.0);
                if (QLineF(b + t * d, p).length() <= radius)
                    return true;
            }
        }
    }
    return false;
}

QJsonArray Stroke::toJson() const
{
    // Flat array [width, x0, y0, p0, x1, y1, p1, ...] keeps files small.
    QJsonArray a;
    a.append(width);
    for (const InkPoint &pt : points) {
        a.append(qRound(pt.x * 100000) / 100000.0);
        a.append(qRound(pt.y * 100000) / 100000.0);
        a.append(qRound(pt.pressure * 1000) / 1000.0);
    }
    return a;
}

Stroke Stroke::fromJson(const QJsonArray &a)
{
    Stroke s;
    if (a.isEmpty())
        return s;
    s.width = float(a.at(0).toDouble(1));
    for (int i = 1; i + 2 < a.size(); i += 3)
        s.points.append({float(a.at(i).toDouble()), float(a.at(i + 1).toDouble()),
                         float(a.at(i + 2).toDouble(1))});
    return s;
}

namespace ink {

static qreal penWidth(const Stroke &s, float pressure, qreal scale)
{
    // Pressure maps to 40%..160% of the base width; never thinner than 1px.
    return qMax<qreal>(1.0, s.width * scale * (0.4 + 1.2 * qBound(0.f, pressure, 1.f)));
}

void paintStroke(QPainter &p, const Stroke &s, qreal scale, int from)
{
    if (s.points.isEmpty())
        return;
    QPen pen(Qt::black);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    if (s.points.size() == 1) {
        const InkPoint &pt = s.points.first();
        pen.setWidthF(penWidth(s, pt.pressure, scale));
        p.setPen(pen);
        p.drawPoint(QPointF(pt.x * scale, pt.y * scale));
        return;
    }
    for (int i = qMax(1, from); i < s.points.size(); ++i) {
        const InkPoint &a = s.points[i - 1];
        const InkPoint &b = s.points[i];
        pen.setWidthF(penWidth(s, (a.pressure + b.pressure) / 2, scale));
        p.setPen(pen);
        p.drawLine(QPointF(a.x * scale, a.y * scale), QPointF(b.x * scale, b.y * scale));
    }
}

QByteArray serialize(const QVector<Stroke> &strokes)
{
    QJsonArray arr;
    for (const Stroke &s : strokes)
        arr.append(s.toJson());
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("strokes"), arr);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QVector<Stroke> deserialize(const QByteArray &data)
{
    QVector<Stroke> out;
    const QJsonArray arr = QJsonDocument::fromJson(data).object().value(QStringLiteral("strokes")).toArray();
    for (const QJsonValue &v : arr) {
        Stroke s = Stroke::fromJson(v.toArray());
        if (!s.points.isEmpty())
            out.append(s);
    }
    return out;
}

}
