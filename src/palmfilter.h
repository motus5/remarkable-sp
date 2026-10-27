#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSet>

class QPointingDevice;
class QWindow;

// Palm rejection: drops touch events while the pen is near the screen.
//
// Device test v0.3.2: a resting hand broke strokes off (only 67 % of the pen
// samples reached the canvas). Qt Quick hands every pointer event to the
// handlers under it; a pointer handler that does not accept the device (the
// pen's PointHandler gets a touch event) calls setActive(false) and so ends
// the running stroke. Filtering touch before Qt Quick sees it avoids that.
//
// Rules: while the pen is in proximity, and for a short grace time after it
// left, touch events are dropped. A touch sequence that Qt Quick already saw
// is cancelled (TouchCancel) when the pen arrives. Every touch point of a
// dropped event stays blocked until it is released, so Qt Quick never sees
// half a sequence.
class PalmFilter : public QObject
{
    Q_OBJECT

public:
    static constexpr int kGraceMs = 300;

    explicit PalmFilter(QObject *parent = nullptr);

    void setPenNear(bool near);
    bool penBlocking() const;
    int droppedEvents() const { return m_dropped; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool m_penNear = false;
    QElapsedTimer m_sinceLeft; // invalid = pen never left
    QSet<int> m_passed;        // touch ids Qt Quick has seen pressed
    QSet<int> m_blocked;       // touch ids dropped until release
    QWindow *m_lastWindow = nullptr;
    const QPointingDevice *m_lastDevice = nullptr;
    int m_dropped = 0;
};
