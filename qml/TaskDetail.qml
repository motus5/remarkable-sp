import QtQuick

// Full-page task view: handwritten title and notes, planning, project, tags.
Rectangle {
    id: root
    property real u: 10
    property string taskId
    property var task: app.get(taskId)
    property bool eraserMode: false
    property bool showDetails: false
    signal closed()

    color: "white"

    function refresh() {
        task = app.get(taskId)
        if (!task.taskId)
            root.closed() // deleted remotely
    }
    function persist() {
        const titleEmpty = titleInk.canvas.empty
        titleInk.canvas.save()
        notesInk.canvas.save()
        app.setTitle(taskId, titleInput.text)
        app.inkSaved(taskId, titleEmpty)
    }
    function close() {
        persist()
        root.closed()
    }

    Connections {
        target: app
        function onDataChanged() { root.refresh() }
        function onTrackingChanged() { root.refresh() }
    }

    Column {
        id: top
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 2 * root.u
        spacing: 1.2 * root.u

        Flow {
            width: parent.width
            spacing: 1.2 * root.u
            EButton { u: root.u; text: "‹ Zurück"; onClicked: root.close() }
            EButton {
                u: root.u
                text: root.task.isDone ? "✓ Erledigt" : "Erledigt"
                checked: root.task.isDone === true
                onClicked: app.toggleDone(root.taskId)
            }
            EButton {
                u: root.u
                visible: !root.task.isDone
                text: (root.task.isTracking ? "❚❚ " : "▶ ") + app.formatDuration(root.task.timeSpent || 0)
                checked: root.task.isTracking === true
                onClicked: app.toggleTracking(root.taskId)
            }
            EButton {
                u: root.u
                visible: !root.task.isSubTask
                text: "+ Unteraufgabe"
                onClicked: {
                    root.persist()
                    root.taskId = app.addSubTask(root.taskId, "")
                }
            }
            EButton { u: root.u; text: "Radierer"; checked: root.eraserMode; onClicked: root.eraserMode = !root.eraserMode }
            EButton {
                u: root.u
                text: "↶"
                enabled: notesInk.canvas.canUndo || titleInk.canvas.canUndo
                onClicked: notesInk.canvas.canUndo ? notesInk.canvas.undo() : titleInk.canvas.undo()
            }
            EButton { u: root.u; text: "Löschen"; onClicked: { app.removeTask(root.taskId); root.closed() } }
        }

        // Planning row: like SP's "schedule" shortcuts.
        Flow {
            width: parent.width
            spacing: root.u
            visible: !root.task.isSubTask
            Text { text: "Planen:"; font.pixelSize: 2.6 * root.u; height: 5.5 * root.u; verticalAlignment: Text.AlignVCenter }
            Chip { u: root.u; text: "Heute"; checked: root.task.isToday === true; onClicked: app.schedule(root.taskId, 0) }
            Chip {
                u: root.u; text: "Morgen"
                checked: root.task.dueDay === Qt.formatDate(new Date(Date.now() + 864e5), "yyyy-MM-dd")
                onClicked: app.schedule(root.taskId, 1)
            }
            Chip { u: root.u; text: "+1 Woche"; onClicked: app.schedule(root.taskId, 7) }
            Chip { u: root.u; text: "Ohne"; checked: !root.task.dueDay && !root.task.dueWithTime; onClicked: app.schedule(root.taskId, -1) }
            Text {
                visible: !!root.task.dueDay && !root.task.isToday
                text: "📅 " + app.formatDay(root.task.dueDay || "")
                font.pixelSize: 2.6 * root.u
                height: 5.5 * root.u
                verticalAlignment: Text.AlignVCenter
            }
        }

        Flow {
            width: parent.width
            spacing: root.u
            Chip {
                u: root.u
                text: "Schätzung " + app.formatDuration(root.task.timeEstimate || 0)
                onClicked: {
                    const steps = [0, 15, 30, 60, 120, 240]
                    const cur = Math.round((root.task.timeEstimate || 0) / 60000)
                    const i = steps.indexOf(cur)
                    app.setEstimateMinutes(root.taskId, steps[(i + 1) % steps.length])
                }
            }
            Chip {
                u: root.u
                text: "Priorität " + (root.task.priority ? "!".repeat(root.task.priority) : "–")
                onClicked: app.setPriority(root.taskId, ((root.task.priority || 0) + 1) % 4)
            }
            Chip {
                u: root.u
                visible: !root.task.isSubTask
                text: (root.task.projectTitle || "Kein Projekt") + (root.showDetails ? "  ▴" : "  ▾")
                checked: root.showDetails
                onClicked: root.showDetails = !root.showDetails
            }
        }

        // Project & tag assignment (collapsed by default to keep the page calm).
        Column {
            width: parent.width
            spacing: root.u
            visible: root.showDetails && !root.task.isSubTask
            Flow {
                width: parent.width
                spacing: root.u
                Text { text: "Projekt:"; font.pixelSize: 2.6 * root.u; height: 5.5 * root.u; verticalAlignment: Text.AlignVCenter }
                Repeater {
                    model: app.projects
                    Chip {
                        u: root.u
                        text: modelData.title
                        checked: root.task.projectId === modelData.id
                        onClicked: app.setProject(root.taskId, modelData.id)
                    }
                }
            }
            Flow {
                width: parent.width
                spacing: root.u
                visible: app.tags.length > 0
                Text { text: "Tags:"; font.pixelSize: 2.6 * root.u; height: 5.5 * root.u; verticalAlignment: Text.AlignVCenter }
                Repeater {
                    model: app.tags
                    Chip {
                        u: root.u
                        text: "# " + modelData.title
                        checked: (root.task.tagIds || []).indexOf(modelData.id) >= 0
                        onClicked: app.toggleTag(root.taskId, modelData.id)
                    }
                }
            }
        }

        InkField {
            id: titleInk
            width: parent.width
            height: 11 * root.u
            u: root.u
            eraserMode: root.eraserMode
            source: root.task.inkTitlePath || ""
            placeholder: "Aufgabe hier hinschreiben …"
        }

        Field {
            id: titleInput
            width: parent.width
            u: root.u
            text: root.task.title === "✍ Handschrift (reMarkable)" ? "" : (root.task.title || "")
            placeholder: "… oder Titel tippen (wird mit SP synchronisiert)"
            onEdited: (t) => app.setTitle(root.taskId, t)
        }

        Text {
            width: parent.width
            visible: (root.task.notes || "") !== ""
            text: root.task.notes || ""
            wrapMode: Text.Wrap
            maximumLineCount: 5
            elide: Text.ElideRight
            font.pixelSize: 2.5 * root.u
            color: "#333333"
        }
    }

    InkField {
        id: notesInk
        anchors.top: top.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2 * root.u
        anchors.topMargin: root.u
        u: root.u
        eraserMode: root.eraserMode
        source: root.task.inkNotesPath || ""
        placeholder: "Notizen, Skizzen, Checklisten …"

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
