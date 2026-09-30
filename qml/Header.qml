import QtQuick

// Status bar: open tasks, time tracked today, running task and focus timer.
Rectangle {
    id: root
    property real u: 10
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

        Row {
            width: parent.width
            spacing: 2 * root.u
            Text {
                text: "Heute"
                font.pixelSize: 5 * root.u
                font.bold: true
            }
            Text {
                anchors.baseline: parent.children[0].baseline
                text: tasks.openCount + " offen · " + tasks.doneTodayCount + " erledigt · "
                      + tasks.formatDuration(tasks.todayTotal) + " erfasst"
                font.pixelSize: 2.8 * root.u
            }
        }

        Row {
            width: parent.width
            spacing: 2 * root.u
            EButton {
                u: root.u
                text: tasks.focusActive ? "Fokus " + tasks.focusRemaining + " min" : "Fokus " + tasks.focusMinutes + " min"
                checked: tasks.focusActive
                onClicked: tasks.focusActive ? tasks.stopFocus() : tasks.startFocus()
            }
            EButton {
                u: root.u
                visible: !tasks.focusActive
                text: "−5"
                onClicked: tasks.focusMinutes = tasks.focusMinutes - 5
            }
            EButton {
                u: root.u
                visible: !tasks.focusActive
                text: "+5"
                onClicked: tasks.focusMinutes = tasks.focusMinutes + 5
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - x
                elide: Text.ElideRight
                visible: tasks.currentTaskId !== ""
                text: "▶ " + (tasks.currentTaskTitle !== "" ? tasks.currentTaskTitle : "(handschriftliche Aufgabe)")
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
