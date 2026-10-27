#pragma once

#include <QColor>
#include <QQuickPaintedItem>

// Line icons in the style of the reMarkable UI, drawn with QPainter so they
// stay crisp at any size and need no SVG/image plugins on the device.
class Icon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)

public:
    explicit Icon(QQuickItem *parent = nullptr);

    QString name() const { return m_name; }
    void setName(const QString &n);
    QColor color() const { return m_color; }
    void setColor(const QColor &c);

    void paint(QPainter *p) override;
    static QStringList names();

signals:
    void changed();

private:
    QString m_name;
    QColor m_color = Qt::black;
};
