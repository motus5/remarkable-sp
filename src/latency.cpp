#include "latency.h"

#include <QCoreApplication>
#include <QTabletEvent>
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
    window->installEventFilter(this);
}

bool LatencyProbe::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletMove) {
        const auto *te = static_cast<QTabletEvent *>(event);
        if (te->buttons() & Qt::LeftButton) {
            QMutexLocker l(&m_mutex);
            ++m_stroke.window;
        }
    }
    return QObject::eventFilter(watched, event);
}

void LatencyProbe::rawSample(qint64 kernelUs, qint64 readUs)
{
    QMutexLocker l(&m_mutex);
    if (!m_penDown)
        ++m_penStroke;
    m_penDown = true;
    m_pendingCanvas.append({kernelUs, m_penStroke});
    ++m_rawPerPenStroke[m_penStroke];
    m_stroke.toApp.append(readUs - kernelUs);
}

void LatencyProbe::penLifted()
{
    QMutexLocker l(&m_mutex);
    m_penDown = false;
    m_liftedStroke = m_penStroke;
}

void LatencyProbe::arrived()
{
    QMutexLocker l(&m_mutex);
    if (m_pendingCanvas.isEmpty())
        return; // mouse, or more canvas calls than pen samples
    const Pending p = m_pendingCanvas.takeFirst();
    const qint64 t0 = p.t0;
    if (m_canvasStroke < 0)
        m_canvasStroke = p.penStroke;
    ++m_stroke.arrived;
    m_stroke.toCanvas.append(nowUs() - t0);
    m_pendingFrame.append(t0);
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
        if (m_canvasStroke < 0 && m_stroke.arrived == 0)
            return; // mouse stroke
        // Aborted: the canvas ended its stroke before the pen was lifted.
        // Otherwise drop the samples of that pen stroke that never arrived.
        if (m_canvasStroke >= 0 && m_liftedStroke < m_canvasStroke)
            ++m_stroke.aborted;
        else
            while (!m_pendingCanvas.isEmpty() && m_pendingCanvas.first().penStroke <= m_canvasStroke)
                m_pendingCanvas.removeFirst();
        m_stroke.raw = takeRawUpTo(m_canvasStroke, &m_stroke.lost);
        m_canvasStroke = -1;
        line = format(m_stroke, false);
        m_total.raw += m_stroke.raw;
        m_total.window += m_stroke.window;
        m_total.aborted += m_stroke.aborted;
        m_total.lost += m_stroke.lost;
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

int LatencyProbe::takeRawUpTo(int penStroke, int *lostStrokes)
{
    // Raw samples of this pen stroke, plus those of earlier pen strokes that
    // never started a canvas stroke (counted as lost).
    int raw = 0;
    for (auto it = m_rawPerPenStroke.begin(); it != m_rawPerPenStroke.end();) {
        if (it.key() > penStroke) {
            ++it;
            continue;
        }
        raw += it.value();
        if (it.key() < penStroke && it.value() > 0)
            ++*lostStrokes;
        it = m_rawPerPenStroke.erase(it);
    }
    // A stroke that restarts within the same pen stroke gets the rest.
    if (m_penDown && penStroke == m_penStroke)
        m_rawPerPenStroke[penStroke] = 0;
    return raw;
}

QString LatencyProbe::summary() const
{
    QMutexLocker l(&m_mutex);
    Stats total = m_total;
    for (auto it = m_rawPerPenStroke.cbegin(); it != m_rawPerPenStroke.cend(); ++it) {
        total.raw += it.value();
        if (it.value() > 0)
            ++total.lost;
    }
    return format(total, true);
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
    return QStringLiteral("%1 roh=%2 fenster=%3 angekommen=%4 dichte=%5% abbruch=%6 verloren=%7 evt_app_ms[%8] "
                          "evt_canvas_ms[%9] evt_frame_ms[%10] frames=%11 render_ms[%12] malen_us_je_punkt=%13")
        .arg(total ? QStringLiteral("LATENZ-SUMME") : QStringLiteral("LATENZ"))
        .arg(s.raw)
        .arg(s.window)
        .arg(s.arrived)
        .arg(density, 0, 'f', 1)
        .arg(s.aborted)
        .arg(s.lost)
        .arg(stat(s.toApp), stat(s.toCanvas), stat(s.toFrame))
        .arg(s.frames)
        .arg(stat(s.render))
        .arg(s.paintCount ? s.paintNs / s.paintCount / 1000.0 : 0, 0, 'f', 1);
}
