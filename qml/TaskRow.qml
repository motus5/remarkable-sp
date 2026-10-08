import QtQuick
import RemarkableSP.Core

// List entry in the calm style of the reMarkable list view: square checkbox,
// title (typed or handwritten), quiet meta line, time and a round play button.
Item {
    id: root
    property real u: 10
    property string taskId
    property string title
    property bool isDone
    property bool isSubTask
    property bool isBacklog
    property bool isTracking
    property real timeSpent
    property real timeEstimate
    property string dueDay
    property int priority
    property string projectTitle
    property string tagTitles
    property string inkTitlePath
    property int inkRevision
    signal opened()
    signal menuRequested(var anchor)

    onInkRevisionChanged: inkTitle.canvas.reload()

    readonly property string meta: {
        const parts = []
        if (isBacklog) parts.push("Backlog")
        if (dueDay && !(app.contextType === "TAG" && app.contextId === "TODAY")) parts.push(app.formatDay(dueDay))
        if (projectTitle) parts.push(projectTitle)
        if (tagTitles) parts.push("# " + tagTitles)
        if (timeSpent > 0 || timeEstimate > 0)
            parts.push(app.formatDuration(timeSpent) + (timeEstimate > 0 ? " von " + app.formatDuration(timeEstimate) : ""))
        return parts.join("   ·   ")
    }
    readonly property bool handwritten: title === "" || title === "✍ Handschrift (reMarkable)"

    height: (meta !== "" ? 14 : 11) * u

    // Running task: black bar at the left edge (crisper on e-ink than grey fills).
    Rectangle { visible: root.isTracking; x: 0; width: 1.2 * root.u; height: parent.height; color: Theme.ink }

    Rectangle {
        id: check
        x: (root.isSubTask ? 10 : 4) * root.u
        anchors.verticalCenter: parent.verticalCenter
        width: 5.4 * root.u
        height: width
        radius: 0.5 * root.u
        border.width: 2.5
        border.color: Theme.ink
        color: root.isDone ? Theme.ink : Theme.paper
        Icon { anchors.centerIn: parent; width: parent.width * 0.8; height: width; name: "check"; color: Theme.paper; visible: root.isDone }
        TapHandler { margin: 1.5 * root.u; onTapped: app.toggleDone(root.taskId) }
    }

    Item {
        id: body
        anchors.left: check.right
        anchors.right: play.left
        anchors.leftMargin: 3 * root.u
        anchors.rightMargin: 2 * root.u
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Text {
            id: titleText
            width: parent.width
            y: root.meta !== "" ? 2.4 * root.u : (parent.height - height) / 2
            visible: !root.handwritten
            elide: Text.ElideRight
            text: (root.priority > 0 ? "!".repeat(root.priority) + "  " : "") + root.title
            font.pixelSize: 3.4 * root.u
            font.strikeout: root.isDone
            color: root.isDone ? Theme.muted : Theme.ink
        }
        InkField {
            id: inkTitle
            width: parent.width
            height: 9.5 * root.u
            y: root.meta !== "" ? 0 : (parent.height - height) / 2
            visible: root.handwritten
            editable: false
            u: root.u
            source: root.handwritten ? root.inkTitlePath : ""
        }
        Text {
            width: parent.width
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2 * root.u
            visible: root.meta !== ""
            text: root.meta
            elide: Text.ElideRight
            font.pixelSize: 2.4 * root.u
            color: Theme.muted
        }
        TapHandler {
            onTapped: root.opened()
            onLongPressed: root.menuRequested(body)
        }
    }

    Rectangle {
        id: play
        visible: !root.isDone
        anchors.right: parent.right
        anchors.rightMargin: 4 * root.u
        anchors.verticalCenter: parent.verticalCenter
        width: 7.4 * root.u
        height: width
        radius: width / 2
        color: root.isTracking ? Theme.ink : Theme.paper
        border.color: Theme.ink
        border.width: 2
        Icon {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.isTracking ? 0 : 0.2 * root.u
            width: 3.8 * root.u; height: width
            name: root.isTracking ? "pause" : "play"
            color: root.isTracking ? Theme.paper : Theme.ink
        }
        TapHandler { margin: root.u; onTapped: app.toggleTracking(root.taskId) }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        x: check.x
        width: parent.width - x - 4 * root.u
        height: 1
        color: Theme.rule
    }
}
