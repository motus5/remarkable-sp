import QtQuick
import RemarkableSP.Core

// The task list pages like a notebook: swipe up/down (finger) or tap the
// arrows; no kinetic scrolling, so the e-ink screen redraws once per page.
Item {
    id: root
    property real u: 10
    signal openTask(string id)
    signal openMenu(var items, var anchor, string title)

    readonly property int pageCount: Math.max(1, Math.ceil((list.contentHeight - 1) / list.height))
    readonly property int pageIndex: Math.min(pageCount - 1, Math.round(list.contentY / list.height))

    function page(dir) {
        if (dir > 0) {
            // First row that is not fully visible becomes the top row.
            const bottom = list.contentY + list.height
            let idx = list.indexAt(10, bottom - 2)
            if (idx < 0) {
                list.contentY = Math.max(0, list.contentHeight - list.height)
                return
            }
            const item = list.itemAtIndex(idx)
            if (item && item.y <= list.contentY + 1)
                idx += 1
            list.positionViewAtIndex(Math.min(idx, list.count - 1), ListView.Beginning)
        } else {
            let y = Math.max(0, list.contentY - list.height)
            const idx = list.indexAt(10, y + 1)
            if (idx <= 0)
                list.contentY = 0
            else
                list.positionViewAtIndex(idx + 1, ListView.Beginning)
        }
    }

    function rowMenu(id) {
        const t = app.get(id)
        return [
            { text: t.isDone ? "Wieder öffnen" : "Erledigt", icon: "check", action: () => app.toggleDone(id) },
            { text: t.isTracking ? "Zeiterfassung stoppen" : "Zeit erfassen", icon: t.isTracking ? "pause" : "play", action: () => app.toggleTracking(id) },
            { separator: true },
            { text: "Heute", icon: "today", checked: t.isToday, action: () => app.schedule(id, 0) },
            { text: "Morgen", icon: "calendar", action: () => app.schedule(id, 1) },
            { text: "Nächste Woche", icon: "calendar", action: () => app.schedule(id, 7) },
            { separator: true },
            { text: "Öffnen", icon: "chevron-right", action: () => root.openTask(id) },
            { text: "Löschen", icon: "trash", action: () => app.removeTask(id) },
        ]
    }

    ListView {
        id: list
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        clip: true
        interactive: false
        model: app.tasks
        delegate: TaskRow {
            width: list.width
            u: root.u
            taskId: model.taskId
            title: model.title
            isDone: model.isDone
            isSubTask: model.isSubTask
            isBacklog: model.isBacklog
            isTracking: model.isTracking
            timeSpent: model.timeSpent
            timeEstimate: model.timeEstimate
            dueDay: model.dueDay
            priority: model.priority
            projectTitle: model.projectTitle
            tagTitles: model.tagTitles
            inkTitlePath: model.inkTitlePath
            inkRevision: model.inkRevision
            onOpened: root.openTask(model.taskId)
            onMenuRequested: (anchor) => root.openMenu(root.rowMenu(model.taskId), anchor, "")
        }

        Column {
            anchors.centerIn: parent
            visible: list.count === 0
            spacing: 2 * root.u
            Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 12 * root.u; height: width; name: app.showDone ? "done-list" : "check"; color: Theme.faint }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: app.showDone ? "Noch nichts erledigt" : "Alles erledigt"
                font.pixelSize: 3.6 * root.u
                color: Theme.muted
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !app.showDone
                text: "Neue Aufgabe mit  +  oben rechts"
                font.pixelSize: 2.6 * root.u
                color: Theme.faint
            }
        }
    }

    // Finger swipe = page turn (the pen never scrolls).
    DragHandler {
        target: null
        acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Mouse
        yAxis.enabled: true
        xAxis.enabled: false
        onActiveChanged: {
            if (active || Math.abs(translation.y) < 6 * root.u)
                return
            root.page(translation.y < 0 ? 1 : -1)
        }
    }

    Item {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 9 * root.u
        visible: root.pageCount > 1

        Row {
            anchors.centerIn: parent
            spacing: 3 * root.u
            IconButton { u: root.u; size: 7 * root.u; icon: "chevron-left"; enabled: root.pageIndex > 0; onClicked: root.page(-1) }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (root.pageIndex + 1) + " / " + root.pageCount
                font.pixelSize: 2.6 * root.u
                color: Theme.muted
            }
            IconButton { u: root.u; size: 7 * root.u; icon: "chevron-right"; enabled: root.pageIndex < root.pageCount - 1; onClicked: root.page(1) }
        }
    }
}
