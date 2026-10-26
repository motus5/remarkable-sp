#include "latency.h"

#include <QCoreApplication>
#include <QQuickWindow>
#include <algorithm>
#include <ctime>

LatencyProbe *LatencyProbe::instance()
{
    static LatencyProbe *probe = [] {
        if (qEnvironmentVariableIntValue("RMSP_LATENCY_LOG") <= 0)
            return static_cast<LatencyProbe *>(nullptr);
        auto *p = new LatencyProbe(QCoreApplication::instance());
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, p,
                         [p] { fprintf(stderr, "%s\n", qPrintable(p->summary())); });
        return p;
    }();
    return probe;
}

LatencyProbe::LatencyProbe(QObject *parent)
    : QObject(parent)
{
}

qint64 LatencyProbe::nowUs()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return qint64(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
}

void LatencyProbe::attach(QQuickWindow *window)
{
    // Direct connections: with a threaded render loop these fire on the render thread.
    connect(window, &QQuickWindow::beforeRendering, this, [this] {
        QMutexLocker l(&m_mutex);
        m_renderStart = nowUs();
    }, Qt::DirectConnection);
    connect(window, &QQuickWindow::frameSwapped, this, &LatencyProbe::frameSwapped, Qt::DirectConnection);
}

void LatencyProbe::rawSample(qint64 kernelUs, qint64 readUs)
{
    QMutexLocker l(&m_mutex);
    m_lastKernel = kernelUs;
    m_lastRead = readUs;
    m_haveRaw = true;
    ++m_stroke.raw;
    m_stroke.toApp.append(readUs - kernelUs);
}

void LatencyProbe::arrived()
{
    QMutexLocker l(&m_mutex);
    if (!m_haveRaw)
        return; // mouse or replay without kernel stamp
    m_haveRaw = false; // count each raw sample at most once
    ++m_stroke.arrived;
    m_stroke.toCanvas.append(nowUs() - m_lastKernel);
    m_pendingFrame.append(m_lastKernel);
}

void LatencyProbe::paintCost(qint64 ns)
{
    QMutexLocker l(&m_mutex);
    m_stroke.paintNs += ns;
    ++m_stroke.paintCount;
}

void LatencyProbe::frameSwapped()
{
    QMutexLocker l(&m_mutex);
    const qint64 now = nowUs();
    if (m_renderStart)
        m_stroke.render.append(now - m_renderStart);
    m_renderStart = 0;
    if (m_pendingFrame.isEmpty())
        return;
    ++m_stroke.frames;
    for (qint64 t0 : std::as_const(m_pendingFrame))
        m_stroke.toFrame.append(now - t0);
    m_pendingFrame.clear();
}

static void append(QVector<qint64> &to, const QVector<qint64> &from)
{
    to += from;
}

void LatencyProbe::strokeEnded()
{
    QString line;
    {
        QMutexLocker l(&m_mutex);
        if (m_stroke.raw == 0 && m_stroke.arrived == 0)
            return;
        line = format(m_stroke, false);
        m_total.raw += m_stroke.raw;
        m_total.arrived += m_stroke.arrived;
        m_total.frames += m_stroke.frames;
        append(m_total.toApp, m_stroke.toApp);
        append(m_total.toCanvas, m_stroke.toCanvas);
        append(m_total.toFrame, m_stroke.toFrame);
        append(m_total.render, m_stroke.render);
        m_total.paintNs += m_stroke.paintNs;
        m_total.paintCount += m_stroke.paintCount;
        m_stroke = Stats();
    }
    // Frames for the last samples may still be pending; they count for the next stroke.
    fprintf(stderr, "%s\n", qPrintable(line));
}

QString LatencyProbe::summary() const
{
    QMutexLocker l(&m_mutex);
    return format(m_total, true);
}

QString LatencyProbe::format(const Stats &s, bool total)
{
    auto stat = [](QVector<qint64> v) {
        if (v.isEmpty())
            return QStringLiteral("avg=- p95=- max=-");
        std::sort(v.begin(), v.end());
        qint64 sum = 0;
        for (qint64 x : std::as_const(v))
            sum += x;
        const qint64 p95 = v.at(qMin(v.size() - 1, int(v.size() * 0.95)));
        return QStringLiteral("avg=%1 p95=%2 max=%3")
            .arg(sum / v.size() / 1000.0, 0, 'f', 1)
            .arg(p95 / 1000.0, 0, 'f', 1)
            .arg(v.last() / 1000.0, 0, 'f', 1);
    };
    const double density = s.raw ? 100.0 * s.arrived / s.raw : 0;
    return QStringLiteral("%1 roh=%2 angekommen=%3 dichte=%4% evt_app_ms[%5] evt_canvas_ms[%6] evt_frame_ms[%7] "
                          "frames=%8 render_ms[%9] malen_us_je_punkt=%10")
        .arg(total ? QStringLiteral("LATENZ-SUMME") : QStringLiteral("LATENZ"))
        .arg(s.raw)
        .arg(s.arrived)
        .arg(density, 0, 'f', 1)
        .arg(stat(s.toApp), stat(s.toCanvas), stat(s.toFrame))
        .arg(s.frames)
        .arg(stat(s.render))
        .arg(s.paintCount ? s.paintNs / s.paintCount / 1000.0 : 0, 0, 'f', 1);
}
