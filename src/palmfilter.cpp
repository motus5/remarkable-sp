#include "palmfilter.h"

#include <QCoreApplication>
#include <QTouchEvent>
#include <QWindow>

PalmFilter::PalmFilter(QObject *parent)
    : QObject(parent)
{
}

bool PalmFilter::penBlocking() const
{
    return m_penNear || (m_sinceLeft.isValid() && m_sinceLeft.elapsed() < kGraceMs);
}

void PalmFilter::setPenNear(bool near)
{
    if (near == m_penNear)
        return;
    m_penNear = near;
    if (!near) {
        m_sinceLeft.start();
        return;
    }
    // Pen arrives while a touch is held: end that sequence for Qt Quick and
    // drop the rest of it.
    if (!m_passed.isEmpty() && m_lastWindow && m_lastDevice) {
        QTouchEvent cancel(QEvent::TouchCancel, m_lastDevice);
        QCoreApplication::sendEvent(m_lastWindow, &cancel);
        m_blocked.unite(m_passed);
        m_passed.clear();
    }
}

bool PalmFilter::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        break;
    default:
        return QObject::eventFilter(watched, event);
    }
    auto *te = static_cast<QTouchEvent *>(event);
    if (auto *w = qobject_cast<QWindow *>(watched))
        m_lastWindow = w;
    m_lastDevice = te->pointingDevice();

    bool drop = penBlocking();
    for (const QEventPoint &p : te->points())
        if (m_blocked.contains(p.id()))
            drop = true;

    for (const QEventPoint &p : te->points()) {
        const bool released = p.state() == QEventPoint::State::Released;
        if (drop) {
            m_passed.remove(p.id());
            if (released)
                m_blocked.remove(p.id());
            else
                m_blocked.insert(p.id());
        } else if (released) {
            m_passed.remove(p.id());
        } else {
            m_passed.insert(p.id());
        }
    }
    if (drop) {
        ++m_dropped;
        event->accept(); // no mouse synthesis from a dropped touch
        return true;
    }
    return false;
}
