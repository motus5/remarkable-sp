import QtQuick

// Task list with paging buttons: flicking works, but whole-page jumps cause far
// fewer e-ink refreshes than kinetic scrolling.
Item {
    id: root
    property real u: 10
    signal openTask(string id)
    signal message(string text)

    ListView {
        id: list
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        clip: true
        model: tasks
        boundsBehavior: Flickable.StopAtBounds
        flickDeceleration: 20000
        maximumFlickVelocity: 4000
        delegate: TaskRow {
            width: list.width
            u: root.u
            taskId: model.taskId
            title: model.title
            isDone: model.isDone
            isSubTask: model.isSubTask
            isTracking: model.isTracking
            timeSpent: model.timeSpent
            timeEstimate: model.timeEstimate
            inkTitlePath: model.inkTitlePath
            inkRevision: model.inkRevision
            onOpened: root.openTask(model.taskId)
        }

        Text {
            anchors.centerIn: parent
            visible: list.count === 0
            text: tasks.showDone ? "Noch nichts erledigt." : "Keine offenen Aufgaben.\nMit „+ Aufgabe“ loslegen."
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 3.4 * root.u
            color: "#666666"
        }

        function page(dir) {
            contentY = Math.max(0, Math.min(contentHeight - height, contentY + dir * height * 0.9))
        }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 11 * root.u
        color: "white"
        Rectangle { width: parent.width; height: Math.max(2, root.u / 4); color: "black" }

        Row {
            anchors.centerIn: parent
            spacing: root.u
            EButton {
                u: root.u
                text: "+ Aufgabe"
                checked: true
                onClicked: root.openTask(tasks.addTask(""))
            }
            EButton { u: root.u; text: "Offen"; checked: !tasks.showDone; onClicked: tasks.showDone = false }
            EButton { u: root.u; text: "Erledigt"; checked: tasks.showDone; onClicked: tasks.showDone = true }
            EButton { u: root.u; text: "▲"; onClicked: list.page(-1) }
            EButton { u: root.u; text: "▼"; onClicked: list.page(1) }
            EButton { u: root.u; text: "Import"; onClicked: root.message(tasks.importFromInbox()) }
            EButton { u: root.u; text: "Export"; onClicked: root.message(tasks.exportNow()) }
        }
    }
}
