#pragma once

#include <QObject>
#include <QPointF>

class QPointingDevice;
class QSocketNotifier;

// Reads the Wacom digitizer of the reMarkable 1/2 straight from evdev and
// feeds Qt tablet events (pen and eraser, with pressure), because the
// reMarkable "epaper" platform plugin only handles touch and keys.
//
// The digitizer is mounted rotated against the portrait screen. The mapping
// defaults to the one used by community tools for rM1/rM2 and can be changed
// with RMSP_PEN_TRANSFORM (comma separated: swapxy, invx, invy), applied in
// that order on raw coordinates normalised to 0..1.
class PenInput : public QObject
{
    Q_OBJECT

public:
    explicit PenInput(QObject *parent = nullptr);
    ~PenInput() override;

    // Opens `path`, or searches /dev/input for a pen digitizer when empty.
    bool open(const QString &path = {});
    QString devicePath() const { return m_path; }

    // Raw evdev event handling, public for tests.
    void setRanges(int maxX, int maxY, int maxPressure);
    void setTransform(const QString &spec);
    void feed(int type, int code, int value);
    QPointF mapToScreen(int rawX, int rawY, const QSizeF &screen) const;

    static QString defaultTransform() { return QStringLiteral("swapxy,invy"); }

private:
    void readEvents();
    void flush();

    int m_fd = -1;
    QString m_path;
    QSocketNotifier *m_notifier = nullptr;
    QPointingDevice *m_pen = nullptr;
    QPointingDevice *m_eraser = nullptr;

    int m_maxX = 20967, m_maxY = 15725, m_maxPressure = 4095; // rM1/rM2 Wacom defaults
    bool m_swap = true, m_invX = false, m_invY = true;

    int m_x = 0, m_y = 0, m_pressure = 0;
    bool m_touching = false, m_wasTouching = false;
    bool m_rubber = false;
    bool m_inProximity = false, m_wasInProximity = false;
};
