#pragma once

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
    void arrived();                                 // canvas consumed the newest sample
    void paintCost(qint64 ns);
    void strokeEnded();
    QString summary() const;

private:
    explicit LatencyProbe(QObject *parent = nullptr);
    void frameSwapped();

    struct Stats {
        int raw = 0, arrived = 0, frames = 0;
        QVector<qint64> toApp, toCanvas, toFrame, render;
        qint64 paintNs = 0;
        int paintCount = 0;
    };
    static QString format(const Stats &s, bool total);

    mutable QMutex m_mutex;
    qint64 m_lastKernel = 0, m_lastRead = 0;
    bool m_haveRaw = false;
    QVector<qint64> m_pendingFrame; // t0 of samples waiting for a frame
    qint64 m_renderStart = 0;
    Stats m_stroke, m_total;
};
