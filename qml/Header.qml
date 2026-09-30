import QtQuick

// Top bar: menu, context title, day stats, focus timer, sync state.
Rectangle {
    id: root
    property real u: 10
    signal menuRequested()
    implicitHeight: col.implicitHeight + 3 * u
    color: "white"

    Column {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 2 * root.u
        anchors.rightMargin: 2 * root.u
        spacing: root.u

        Item {
            width: parent.width
            height: 7 * root.u
            EButton {
                id: menuBtn
                u: root.u
                text: "☰"
                implicitWidth: 7 * root.u
                onClicked: root.menuRequested()
            }
            Text {
                anchors.left: menuBtn.right
                anchors.leftMargin: 2 * root.u
                anchors.right: syncBtn.left
                anchors.verticalCenter: parent.verticalCenter
                text: app.contextTitle
                elide: Text.ElideRight
                font.pixelSize: 4.6 * root.u
                font.bold: true
            }
            EButton {
                id: syncBtn
                anchors.right: parent.right
                u: root.u
                visible: sync.configured
                text: sync.busy ? "…" : (!sync.lastOk ? "⚠ Sync" : (app.pendingCount > 0 ? "↻ " + app.pendingCount : "↻"))
                onClicked: sync.syncNow()
            }
        }

        Text {
            width: parent.width
            elide: Text.ElideRight
            text: app.openCount + " offen · " + app.doneTodayCount + " erledigt · "
                  + app.formatDuration(app.todayTotal) + " erfasst"
                  + (app.todayEstimate > 0 ? " · noch ~" + app.formatDuration(app.todayEstimate) : "")
            font.pixelSize: 2.6 * root.u
        }

        Row {
            width: parent.width
            spacing: 1.5 * root.u
            EButton {
                u: root.u
                text: app.focusActive ? "Fokus " + app.focusRemaining + " min" : "Fokus " + app.focusMinutes + " min"
                checked: app.focusActive
                onClicked: app.focusActive ? app.stopFocus() : app.startFocus()
            }
            EButton { u: root.u; visible: !app.focusActive; text: "−5"; onClicked: app.focusMinutes = app.focusMinutes - 5 }
            EButton { u: root.u; visible: !app.focusActive; text: "+5"; onClicked: app.focusMinutes = app.focusMinutes + 5 }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - x
                elide: Text.ElideRight
                visible: app.currentTaskId !== ""
                text: "▶ " + app.currentTaskTitle
                font.pixelSize: 2.8 * root.u
                font.bold: true
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Math.max(2, root.u / 4)
        color: "black"
    }
}
