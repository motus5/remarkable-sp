#pragma once

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QVector>

class QQuickWindow;

// Pen latency probe, enabled with RMSP_LATENCY_LOG=1 (device test v0.3.1
// reported ~400 ms by eye; this turns it into numbers).
//
// Per pen sample it records, on CLOCK_MONOTONIC in microseconds:
//   t0  kernel time stamp of the evdev sample (EVIOCSCLOCKID monotonic)
//   t1  sample read by the app
//   t1b sample delivered to the window as QTabletEvent ("fenster")
//   t2  sample arrived at the ink canvas
//   t3  first frame swapped after t2 (scene graph done; the e-ink panel
//       update that follows is hardware and not visible to the app)
// After every stroke one line "LATENZ ..." is written to stderr, at exit
// a summary "LATENZ-SUMME ...".
class LatencyProbe : public QObject
{
    Q_OBJECT

public:
    static LatencyProbe *instance(); // nullptr when disabled
    static qint64 nowUs();

    void attach(QQuickWindow *window);
    void rawSample(qint64 kernelUs, qint64 readUs); // pen touching, per SYN
    void penLifted();                               // pen left the surface
    void arrived();                                 // canvas consumed the oldest pending sample
    void paintCost(qint64 ns);
    void strokeEnded();
    QString summary() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit LatencyProbe(QObject *parent = nullptr);
    void frameSwapped();

    struct Stats {
        int raw = 0, window = 0, arrived = 0, frames = 0;
        int aborted = 0; // canvas stroke ended while the pen was still down
        int lost = 0;    // pen strokes of which no sample reached the canvas
        QVector<qint64> toApp, toCanvas, toFrame, render;
        qint64 paintNs = 0;
        int paintCount = 0;
    };
    static QString format(const Stats &s, bool total);

    mutable QMutex m_mutex;
    // t0 of raw samples not yet consumed by the canvas, oldest first. A read
    // can return several samples at once (v0.3.2 counted only one of them).
    struct Pending {
        qint64 t0;
        int penStroke; // number of the pen-down period the sample belongs to
    };
    QVector<Pending> m_pendingCanvas;
    bool m_penDown = false;
    int m_penStroke = 0;     // pen-down periods so far
    int m_liftedStroke = 0;  // last pen-down period that ended
    int m_canvasStroke = -1; // pen stroke of the running canvas stroke
    QHash<int, int> m_rawPerPenStroke;
    int takeRawUpTo(int penStroke, int *lostStrokes); // needs m_mutex
    QVector<qint64> m_pendingFrame; // t0 of samples waiting for a frame
    qint64 m_renderStart = 0;
    Stats m_stroke, m_total;
};
