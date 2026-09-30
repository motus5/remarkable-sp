import QtQuick

// One list entry: checkbox, title (typed or handwritten), meta line, time, play/pause.
Rectangle {
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

    onInkRevisionChanged: inkTitle.canvas.reload()

    readonly property string meta: {
        const parts = []
        if (isBacklog) parts.push("Backlog")
        if (dueDay && !(app.contextType === "TAG" && app.contextId === "TODAY")) parts.push("📅 " + app.formatDay(dueDay))
        if (projectTitle) parts.push(projectTitle)
        if (tagTitles) parts.push("# " + tagTitles)
        return parts.join("  ·  ")
    }
    readonly property bool handwritten: title === "" || title === "✍ Handschrift (reMarkable)"

    height: (meta !== "" ? 13 : 11) * u
    color: isTracking ? "#e6e6e6" : "white"

    Rectangle {
        id: check
        x: (root.isSubTask ? 7 : 2) * root.u
        anchors.verticalCenter: parent.verticalCenter
        width: 6 * root.u
        height: width
        border.width: Math.max(2, root.u / 3)
        border.color: "black"
        color: "white"
        Text {
            anchors.centerIn: parent
            visible: root.isDone
            text: "✓"
            font.pixelSize: 4.5 * root.u
            font.bold: true
        }
        TapHandler {
            margin: root.u
            onTapped: app.toggleDone(root.taskId)
        }
    }

    Item {
        id: titleArea
        anchors.left: check.right
        anchors.right: side.left
        anchors.leftMargin: 2 * root.u
        anchors.rightMargin: root.u
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Text {
            id: prio
            anchors.left: parent.left
            anchors.verticalCenter: titleText.verticalCenter
            visible: root.priority > 0
            text: "!".repeat(root.priority) + " "
            font.pixelSize: 3.4 * root.u
            font.bold: true
        }
        Text {
            id: titleText
            anchors.left: prio.visible ? prio.right : parent.left
            anchors.right: parent.right
            y: root.meta !== "" ? 1.5 * root.u : (parent.height - height) / 2
            visible: !root.handwritten
            elide: Text.ElideRight
            text: root.title
            font.pixelSize: 3.4 * root.u
            font.strikeout: root.isDone
        }
        InkField {
            id: inkTitle
            anchors.left: prio.visible ? prio.right : parent.left
            anchors.right: parent.right
            y: root.meta !== "" ? 0 : 0
            height: 9 * root.u
            visible: root.handwritten
            editable: false
            u: root.u
            source: root.handwritten ? root.inkTitlePath : ""
        }
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1.2 * root.u
            visible: root.meta !== ""
            text: root.meta
            elide: Text.ElideRight
            font.pixelSize: 2.3 * root.u
            color: "#444444"
        }
        TapHandler { onTapped: root.opened() }
    }

    Row {
        id: side
        anchors.right: parent.right
        anchors.rightMargin: 2 * root.u
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2 * root.u

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: app.formatDuration(root.timeSpent)
                  + (root.timeEstimate > 0 ? " / " + app.formatDuration(root.timeEstimate) : "")
            font.pixelSize: 2.6 * root.u
            visible: root.timeSpent > 0 || root.timeEstimate > 0
        }
        EButton {
            u: root.u
            visible: !root.isDone
            implicitWidth: 8 * root.u
            text: root.isTracking ? "❚❚" : "▶"
            checked: root.isTracking
            onClicked: app.toggleTracking(root.taskId)
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        x: check.x
        width: parent.width - x - 2 * root.u
        height: 1
        color: "#aaaaaa"
    }
}
