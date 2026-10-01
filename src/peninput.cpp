#include "peninput.h"

#include <QDir>
#include <QGuiApplication>
#include <QPointingDevice>
#include <QScreen>
#include <QSocketNotifier>
#include <QWindow>
#include <qpa/qwindowsysteminterface.h>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifndef EV_SYN
// Non-Linux builds only use feed() in tests; keep the event codes available.
#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_ABS 0x03
#define SYN_REPORT 0
#define BTN_TOOL_PEN 0x140
#define BTN_TOOL_RUBBER 0x141
#define BTN_TOUCH 0x14a
#define ABS_X 0x00
#define ABS_Y 0x01
#define ABS_PRESSURE 0x18
#endif

PenInput::PenInput(QObject *parent)
    : QObject(parent)
{
    const auto caps = QInputDevice::Capability::Position | QInputDevice::Capability::Pressure;
    m_pen = new QPointingDevice(QStringLiteral("reMarkable pen"), 1, QInputDevice::DeviceType::Stylus,
                                QPointingDevice::PointerType::Pen, caps, 1, 2, QString(),
                                QPointingDeviceUniqueId::fromNumericId(1), this);
    m_eraser = new QPointingDevice(QStringLiteral("reMarkable eraser"), 2, QInputDevice::DeviceType::Stylus,
                                   QPointingDevice::PointerType::Eraser, caps, 1, 2, QString(),
                                   QPointingDeviceUniqueId::fromNumericId(2), this);
    QWindowSystemInterface::registerInputDevice(m_pen);
    QWindowSystemInterface::registerInputDevice(m_eraser);
    setTransform(qEnvironmentVariable("RMSP_PEN_TRANSFORM", defaultTransform()));
}

PenInput::~PenInput()
{
#ifdef Q_OS_LINUX
    if (m_fd >= 0)
        ::close(m_fd);
#endif
}

void PenInput::setRanges(int maxX, int maxY, int maxPressure)
{
    m_maxX = qMax(1, maxX);
    m_maxY = qMax(1, maxY);
    m_maxPressure = qMax(1, maxPressure);
}

void PenInput::setTransform(const QString &spec)
{
    const QStringList parts = spec.toLower().split(QLatin1Char(','), Qt::SkipEmptyParts);
    m_swap = parts.contains(QLatin1String("swapxy"));
    m_invX = parts.contains(QLatin1String("invx"));
    m_invY = parts.contains(QLatin1String("invy"));
}

QPointF PenInput::mapToScreen(int rawX, int rawY, const QSizeF &screen) const
{
    qreal x = qreal(rawX) / m_maxX;
    qreal y = qreal(rawY) / m_maxY;
    if (m_swap)
        std::swap(x, y);
    if (m_invX)
        x = 1 - x;
    if (m_invY)
        y = 1 - y;
    return QPointF(qBound<qreal>(0, x, 1) * screen.width(), qBound<qreal>(0, y, 1) * screen.height());
}

bool PenInput::open(const QString &path)
{
#ifdef Q_OS_LINUX
    QStringList candidates;
    if (!path.isEmpty()) {
        candidates << path;
    } else {
        const QDir dir(QStringLiteral("/dev/input"));
        for (const QString &n : dir.entryList({QStringLiteral("event*")}, QDir::System, QDir::Name))
            candidates << dir.filePath(n);
    }
    for (const QString &c : std::as_const(candidates)) {
        const int fd = ::open(QFile::encodeName(c).constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;
        // A pen digitizer reports BTN_TOOL_PEN and absolute pressure.
        unsigned long keys[(KEY_MAX + 1) / (8 * sizeof(long)) + 1] = {};
        input_absinfo ax{}, ay{}, ap{};
        const bool isPen = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) >= 0
            && (keys[BTN_TOOL_PEN / (8 * sizeof(long))] >> (BTN_TOOL_PEN % (8 * sizeof(long))) & 1)
            && ioctl(fd, EVIOCGABS(ABS_X), &ax) >= 0 && ioctl(fd, EVIOCGABS(ABS_Y), &ay) >= 0
            && ioctl(fd, EVIOCGABS(ABS_PRESSURE), &ap) >= 0 && ax.maximum > 0 && ay.maximum > 0;
        if (!isPen) {
            ::close(fd);
            continue;
        }
        setRanges(ax.maximum, ay.maximum, ap.maximum);
        m_fd = fd;
        m_path = c;
        m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, &PenInput::readEvents);
        qInfo("Pen digitizer %s (x 0..%d, y 0..%d, pressure 0..%d), transform %s", qPrintable(c), ax.maximum,
              ay.maximum, ap.maximum, qPrintable(qEnvironmentVariable("RMSP_PEN_TRANSFORM", defaultTransform())));
        return true;
    }
    qWarning("No pen digitizer found in /dev/input");
#else
    Q_UNUSED(path);
#endif
    return false;
}

void PenInput::readEvents()
{
#ifdef Q_OS_LINUX
    input_event ev[64];
    for (;;) {
        const ssize_t n = ::read(m_fd, ev, sizeof(ev));
        if (n <= 0)
            break;
        for (size_t i = 0; i < size_t(n) / sizeof(input_event); ++i)
            feed(ev[i].type, ev[i].code, ev[i].value);
    }
#endif
}

void PenInput::feed(int type, int code, int value)
{
    if (type == EV_ABS) {
        if (code == ABS_X)
            m_x = value;
        else if (code == ABS_Y)
            m_y = value;
        else if (code == ABS_PRESSURE)
            m_pressure = value;
    } else if (type == EV_KEY) {
        if (code == BTN_TOUCH)
            m_touching = value != 0;
        else if (code == BTN_TOOL_PEN || code == BTN_TOOL_RUBBER) {
            m_inProximity = value != 0;
            if (value)
                m_rubber = code == BTN_TOOL_RUBBER;
        }
    } else if (type == EV_SYN && code == SYN_REPORT) {
        flush();
    }
}

void PenInput::flush()
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window && !QGuiApplication::topLevelWindows().isEmpty())
        window = QGuiApplication::topLevelWindows().constFirst();
    if (!window)
        return;
    const QPointingDevice *device = m_rubber ? m_eraser : m_pen;
    const QSizeF size = window->screen() ? QSizeF(window->screen()->geometry().size()) : QSizeF(window->size());
    const QPointF global = mapToScreen(m_x, m_y, size);
    const QPointF local = window->mapFromGlobal(global.toPoint()) + (global - global.toPoint());

    if (m_inProximity && !m_wasInProximity)
        QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(window, device, true);
    if (m_inProximity || m_wasTouching) {
        const qreal pressure = m_touching ? qreal(m_pressure) / m_maxPressure : 0;
        QWindowSystemInterface::handleTabletEvent(window, device, local, global,
                                                  m_touching ? Qt::LeftButton : Qt::NoButton, pressure, 0, 0, 0, 0, 0);
    }
    if (!m_inProximity && m_wasInProximity)
        QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(window, device, false);
    m_wasTouching = m_touching;
    m_wasInProximity = m_inProximity;
}
