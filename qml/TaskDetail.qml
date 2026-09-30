import QtQuick

// Full-page task view: handwritten title + handwritten notes page.
Rectangle {
    id: root
    property real u: 10
    property string taskId
    property var task: tasks.get(taskId)
    property bool eraserMode: false
    signal closed()

    color: "white"

    function refresh() { task = tasks.get(taskId) }
    function persist() {
        titleInk.canvas.save()
        notesInk.canvas.save()
        tasks.setTitle(taskId, titleInput.text)
        tasks.inkSaved(taskId)
    }
    function close() {
        persist()
        root.closed()
    }

    Connections {
        target: tasks
        function onStatsChanged() { root.refresh() }
        function onTrackingChanged() { root.refresh() }
    }

    Flow {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 2 * root.u
        spacing: 1.5 * root.u

        EButton { u: root.u; text: "‹ Zurück"; onClicked: root.close() }
        EButton {
            u: root.u
            text: root.task.isDone ? "✓ Erledigt" : "Erledigt"
            checked: root.task.isDone === true
            onClicked: { tasks.toggleDone(root.taskId); root.refresh() }
        }
        EButton {
            u: root.u
            visible: !root.task.isDone
            text: root.task.isTracking ? "❚❚ " + tasks.formatDuration(root.task.timeSpent)
                                       : "▶ " + tasks.formatDuration(root.task.timeSpent)
            checked: root.task.isTracking === true
            onClicked: tasks.toggleTracking(root.taskId)
        }
        EButton {
            u: root.u
            text: "Schätzung " + tasks.formatDuration(root.task.timeEstimate || 0)
            onClicked: {
                // Cycle through common estimates: 0, 15, 30, 60, 120 minutes.
                const steps = [0, 15, 30, 60, 120]
                const cur = Math.round((root.task.timeEstimate || 0) / 60000)
                const next = steps[(steps.indexOf(cur) + 1) % steps.length]
                tasks.setEstimateMinutes(root.taskId, next)
            }
        }
        EButton {
            u: root.u
            visible: !root.task.isSubTask
            text: "+ Unteraufgabe"
            onClicked: {
                root.persist()
                const id = tasks.addTask("", root.taskId)
                root.taskId = id
                root.refresh()
            }
        }
        EButton { u: root.u; text: "Radierer"; checked: root.eraserMode; onClicked: root.eraserMode = !root.eraserMode }
        EButton {
            u: root.u
            text: "↶"
            enabled: notesInk.canvas.canUndo || titleInk.canvas.canUndo
            onClicked: notesInk.canvas.canUndo ? notesInk.canvas.undo() : titleInk.canvas.undo()
        }
        EButton {
            u: root.u
            text: "Löschen"
            onClicked: { tasks.removeTask(root.taskId); root.closed() }
        }
    }

    Column {
        id: head
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 2 * root.u
        spacing: root.u

        InkField {
            id: titleInk
            width: parent.width
            height: 12 * root.u
            u: root.u
            eraserMode: root.eraserMode
            source: root.task.inkTitlePath || ""
            placeholder: "Aufgabe hier hinschreiben …"
        }

        // Typed title: for the Type Folio keyboard, desktop, or SP imports.
        Rectangle {
            width: parent.width
            height: 6 * root.u
            border.color: "#aaaaaa"
            border.width: 1
            TextInput {
                id: titleInput
                anchors.fill: parent
                anchors.leftMargin: root.u
                verticalAlignment: TextInput.AlignVCenter
                font.pixelSize: 3 * root.u
                text: root.task.title || ""
                onEditingFinished: tasks.setTitle(root.taskId, text)
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !parent.text && !parent.activeFocus
                    text: "… oder tippen (Tastatur / Type Folio)"
                    color: "#888888"
                    font.pixelSize: 2.6 * root.u
                }
            }
        }

        Text {
            width: parent.width
            visible: (root.task.notes || "") !== ""
            text: root.task.notes || ""
            wrapMode: Text.Wrap
            maximumLineCount: 6
            elide: Text.ElideRight
            font.pixelSize: 2.6 * root.u
            color: "#333333"
        }

        Text {
            text: "Notizen"
            font.pixelSize: 2.6 * root.u
            font.bold: true
        }
    }

    InkField {
        id: notesInk
        anchors.top: head.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2 * root.u
        anchors.topMargin: root.u
        u: root.u
        eraserMode: root.eraserMode
        source: root.task.inkNotesPath || ""
        placeholder: "Notizen, Skizzen, Checklisten …"

        // Ruled lines as a writing guide.
        Repeater {
            model: Math.floor(notesInk.height / (8 * root.u))
            Rectangle {
                y: (index + 1) * 8 * root.u
                x: root.u
                width: notesInk.width - 2 * root.u
                height: 1
                color: "#dddddd"
            }
        }
    }
}
