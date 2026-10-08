import QtQuick
import RemarkableSP.Core

// A task opened like a reMarkable notebook page: paper with a handwritten
// title and a notes page (template), the tool column on the left that folds
// away behind the round button, and page turning to the neighbouring tasks.
Rectangle {
    id: root
    property real u: 10
    property string taskId
    property var task: app.get(taskId)
    property bool toolsOpen: true
    property bool eraserMode: false
    property var lastCanvas: notesInk.canvas
    property bool editingTitle: false
    signal closed()
    signal openMenu(var items, var anchor, string title)

    color: Theme.paper

    function refresh() {
        task = app.get(taskId)
        if (!task.taskId)
            root.closed() // deleted, e.g. remotely
    }
    function persist() {
        const titleEmpty = titleInk.canvas.empty
        titleInk.canvas.save()
        notesInk.canvas.save()
        app.setTitle(taskId, titleField.text)
        app.inkSaved(taskId, titleEmpty)
    }
    function close() {
        persist()
        root.closed()
    }
    function turn(delta) {
        const next = app.neighbourTask(taskId, delta)
        if (next === "")
            return
        persist()
        taskId = next
    }
    onTaskIdChanged: refresh()

    Connections {
        target: app
        function onDataChanged() { root.refresh() }
        function onTrackingChanged() { root.refresh() }
    }

    // ---- menus ------------------------------------------------------------
    function penMenu() {
        return [
            { text: "Fein", icon: "pen-fine", checked: app.penWidth === 1, action: () => app.penWidth = 1 },
            { text: "Mittel", icon: "pen", checked: app.penWidth === 2, action: () => app.penWidth = 2 },
            { text: "Breit", icon: "pen-thick", checked: app.penWidth === 3, action: () => app.penWidth = 3 },
        ]
    }
    function templateMenu() {
        const t = [["lined", "Liniert"], ["grid", "Kariert"], ["dots", "Punkte"], ["blank", "Leer"]]
        return t.map(x => ({ text: x[1], icon: "template", checked: app.paperTemplate === x[0], action: () => app.paperTemplate = x[0] }))
    }
    function scheduleMenu() {
        return [
            { text: "Heute", icon: "today", checked: task.isToday, action: () => app.schedule(taskId, 0) },
            { text: "Morgen", icon: "calendar", action: () => app.schedule(taskId, 1) },
            { text: "Übermorgen", icon: "calendar", action: () => app.schedule(taskId, 2) },
            { text: "Nächste Woche", icon: "calendar", action: () => app.schedule(taskId, 7) },
            { separator: true },
            { text: "Nicht geplant", icon: "close", checked: !task.dueDay && !task.dueWithTime, action: () => app.schedule(taskId, -1) },
        ]
    }
    function estimateMenu() {
        const cur = Math.round((task.timeEstimate || 0) / 60000)
        return [0, 15, 30, 45, 60, 90, 120, 180, 240].map(m => ({
            text: m === 0 ? "Keine Schätzung" : app.formatDuration(m * 60000), icon: "estimate",
            checked: cur === m, action: () => app.setEstimateMinutes(taskId, m) }))
    }
    function priorityMenu() {
        return [[0, "Keine"], [1, "Niedrig  !"], [2, "Mittel  !!"], [3, "Hoch  !!!"]].map(p => ({
            text: p[1], icon: "priority", checked: (task.priority || 0) === p[0], action: () => app.setPriority(taskId, p[0]) }))
    }
    function projectMenu() {
        return app.projects.map(p => ({ text: p.title, icon: p.id === "INBOX_PROJECT" ? "inbox" : "project",
                                        checked: task.projectId === p.id, action: () => app.setProject(taskId, p.id) }))
    }
    function tagMenu() {
        return app.tags.map(t => ({ text: t.title, icon: "tag", keepOpen: true,
                                    checked: (task.tagIds || []).indexOf(t.id) >= 0,
                                    action: () => app.toggleTag(taskId, t.id) }))
    }
    function moreMenu() {
        const items = []
        if (!task.isSubTask)
            items.push({ text: "Unteraufgabe hinzufügen", icon: "subtask", action: () => { root.persist(); root.taskId = app.addSubTask(taskId, "") } })
        items.push({ text: "Titel und Notizen-Tinte löschen", icon: "eraser", action: () => { titleInk.canvas.clear(); notesInk.canvas.clear() } })
        items.push({ separator: true })
        items.push({ text: "Aufgabe löschen", icon: "trash", action: () => { app.removeTask(taskId); root.closed() } })
        return items
    }

    // ---- tool column --------------------------------------------------------
    Rectangle {
        id: tools
        visible: root.toolsOpen
        width: 12 * root.u
        height: parent.height
        color: Theme.paper
        z: 2
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.rule }

        Column {
            y: 12 * root.u
            width: parent.width
            spacing: 0.5 * root.u

            component Sep: Rectangle { x: 3 * root.u; width: 6 * root.u; height: 1; color: Theme.rule }

            IconButton {
                id: penBtn
                u: root.u; icon: ["pen-fine", "pen", "pen-thick"][app.penWidth - 1]
                checked: !root.eraserMode
                onClicked: root.eraserMode ? root.eraserMode = false : root.openMenu(root.penMenu(), penBtn, "Stift")
            }
            IconButton { u: root.u; icon: "eraser"; checked: root.eraserMode; onClicked: root.eraserMode = !root.eraserMode }
            IconButton { u: root.u; icon: "undo"; enabled: root.lastCanvas.canUndo; onClicked: root.lastCanvas.undo() }
            IconButton { u: root.u; icon: "redo"; enabled: root.lastCanvas.canRedo; onClicked: root.lastCanvas.redo() }
            IconButton { id: tplBtn; u: root.u; icon: "template"; onClicked: root.openMenu(root.templateMenu(), tplBtn, "Vorlage") }
            Sep {}
            IconButton {
                u: root.u; icon: "check"; checked: root.task.isDone === true
                caption: root.task.isDone ? "Erledigt" : ""
                onClicked: app.toggleDone(root.taskId)
            }
            IconButton {
                u: root.u
                visible: !root.task.isDone
                icon: root.task.isTracking ? "pause" : "play"
                checked: root.task.isTracking === true
                caption: app.formatDuration(root.task.timeSpent || 0)
                onClicked: app.toggleTracking(root.taskId)
            }
            IconButton { id: calBtn; u: root.u; icon: "calendar"; visible: !root.task.isSubTask; onClicked: root.openMenu(root.scheduleMenu(), calBtn, "Planen") }
            IconButton { id: moreBtn; u: root.u; icon: "more"; onClicked: root.openMenu(root.moreMenu(), moreBtn, "") }
        }

        IconButton {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2 * root.u
            u: root.u
            icon: "close"
            caption: "Schließen"
            onClicked: root.close()
        }
    }

    // The round button that folds the tools away, always top-left.
    Rectangle {
        x: 2 * root.u
        y: 2 * root.u
        z: 3
        width: 8 * root.u; height: width; radius: width / 2
        color: Theme.paper
        border.color: Theme.ink; border.width: 2
        Icon { anchors.centerIn: parent; width: 4.4 * root.u; height: width; name: root.toolsOpen ? "chevron-left" : "menu" }
        TapHandler { onTapped: root.toolsOpen = !root.toolsOpen }
    }

    // ---- paper --------------------------------------------------------------
    Item {
        id: paper
        anchors.left: root.toolsOpen ? tools.right : parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.toolsOpen ? 3 * root.u : 12 * root.u
        anchors.rightMargin: 4 * root.u

        Column {
            id: head
            y: 3 * root.u
            width: parent.width
            spacing: 1.2 * root.u

            Text {
                text: (root.task.isSubTask ? "Unteraufgabe · " : "") + (root.task.projectTitle || "")
                font.pixelSize: 2.4 * root.u
                color: Theme.muted
                height: 5 * root.u
                verticalAlignment: Text.AlignVCenter
            }

            // Handwritten title on a heading rule.
            InkField {
                id: titleInk
                width: parent.width - 8 * root.u
                height: 12 * root.u
                u: root.u
                eraserMode: root.eraserMode
                source: root.task.inkTitlePath || ""
                placeholder: titleField.text === "" ? "Titel schreiben …" : ""
                onStroked: root.lastCanvas = titleInk.canvas
                Text { // typed title shows through when there is no ink
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    visible: titleInk.canvas.empty && titleField.text !== "" && !titleField.input.activeFocus
                    text: titleField.text
                    font.pixelSize: 5 * root.u
                    font.weight: Font.DemiBold
                    width: parent.width
                    elide: Text.ElideRight
                }
            }
            Rectangle {
                width: parent.width; height: 3; color: Theme.ink
                // keyboard: type or edit the title that syncs to Super Productivity
                IconButton {
                    anchors.right: parent.right
                    anchors.bottom: parent.top
                    anchors.bottomMargin: 2 * root.u
                    u: root.u
                    size: 8 * root.u
                    icon: "keyboard"
                    checked: titleField.input.activeFocus
                    onClicked: { root.editingTitle = true; titleField.input.forceActiveFocus() }
                }
            }

            Field {
                id: titleField
                visible: root.editingTitle || (titleInk.canvas.empty && text === "")
                width: parent.width
                u: root.u
                fontSize: 2.6 * root.u
                text: root.task.title === "✍ Handschrift (reMarkable)" ? "" : (root.task.title || "")
                placeholder: "Titel tippen (wird mit Super Productivity synchronisiert)"
                onEdited: (t) => { app.setTitle(root.taskId, t); root.editingTitle = false }
            }

            Flow {
                width: parent.width
                spacing: 1.2 * root.u
                Chip {
                    id: dueChip
                    u: root.u
                    visible: !root.task.isSubTask
                    icon: "calendar"
                    text: root.task.dueDay ? app.formatDay(root.task.dueDay) : "Planen"
                    checked: root.task.isToday === true
                    onClicked: root.openMenu(root.scheduleMenu(), dueChip, "Planen")
                }
                Chip {
                    id: estChip
                    u: root.u
                    icon: "estimate"
                    text: (root.task.timeSpent > 0 ? app.formatDuration(root.task.timeSpent) + " / " : "")
                          + (root.task.timeEstimate > 0 ? app.formatDuration(root.task.timeEstimate) : "Schätzung")
                    onClicked: root.openMenu(root.estimateMenu(), estChip, "Zeitschätzung")
                }
                Chip {
                    id: prioChip
                    u: root.u
                    icon: "priority"
                    text: root.task.priority ? "!".repeat(root.task.priority) : "Priorität"
                    onClicked: root.openMenu(root.priorityMenu(), prioChip, "Priorität")
                }
                Chip {
                    id: projChip
                    u: root.u
                    visible: !root.task.isSubTask
                    icon: "project"
                    text: root.task.projectTitle || "Projekt"
                    onClicked: root.openMenu(root.projectMenu(), projChip, "Projekt")
                }
                Chip {
                    id: tagChip
                    u: root.u
                    visible: !root.task.isSubTask && app.tags.length > 0
                    icon: "tag"
                    text: {
                        const ids = root.task.tagIds || []
                        const names = app.tags.filter(t => ids.indexOf(t.id) >= 0).map(t => t.title)
                        return names.length ? names.join(", ") : "Tags"
                    }
                    onClicked: root.openMenu(root.tagMenu(), tagChip, "Tags")
                }
            }

            Text {
                width: parent.width
                visible: (root.task.notes || "") !== ""
                text: root.task.notes || ""
                wrapMode: Text.Wrap
                maximumLineCount: 4
                elide: Text.ElideRight
                font.pixelSize: 2.5 * root.u
                color: Theme.muted
            }
        }

        InkField {
            id: notesInk
            anchors.top: head.bottom
            anchors.topMargin: 2 * root.u
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: pager.top
            u: root.u
            eraserMode: root.eraserMode
            source: root.task.inkNotesPath || ""
            paperTemplate: app.paperTemplate
            lineSpacing: 7 * root.u
            onStroked: root.lastCanvas = notesInk.canvas
        }

        // Page indicator: position of this task in the current list.
        Item {
            id: pager
            anchors.bottom: parent.bottom
            width: parent.width
            height: 8 * root.u
            readonly property int pos: app.positionOf(root.taskId)
            Row {
                anchors.centerIn: parent
                spacing: 3 * root.u
                visible: pager.pos >= 0
                IconButton { u: root.u; size: 6.5 * root.u; icon: "chevron-left"; enabled: pager.pos > 0; onClicked: root.turn(-1) }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (pager.pos + 1) + " / " + app.tasks.count
                    font.pixelSize: 2.5 * root.u
                    color: Theme.muted
                }
                IconButton { u: root.u; size: 6.5 * root.u; icon: "chevron-right"; enabled: pager.pos < app.tasks.count - 1; onClicked: root.turn(1) }
            }
        }
    }

    // Finger swipe left/right turns to the next/previous task, like pages.
    DragHandler {
        target: null
        acceptedDevices: PointerDevice.TouchScreen
        xAxis.enabled: true
        yAxis.enabled: false
        onActiveChanged: {
            if (!active && Math.abs(translation.x) > 10 * root.u)
                root.turn(translation.x < 0 ? 1 : -1)
        }
    }
}
