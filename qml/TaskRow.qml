import QtQuick

// One list entry: checkbox, title (typed or handwritten), time, play/pause.
Rectangle {
    id: root
    property real u: 10
    property string taskId
    property string title
    property bool isDone
    property bool isSubTask
    property bool isTracking
    property real timeSpent
    property real timeEstimate
    property string inkTitlePath
    property int inkRevision
    onInkRevisionChanged: inkTitle.canvas.reload()
    signal opened()

    height: 11 * u
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
            onTapped: tasks.toggleDone(root.taskId)
        }
    }

    Item {
        id: titleArea
        anchors.left: check.right
        anchors.right: meta.left
        anchors.leftMargin: 2 * root.u
        anchors.rightMargin: root.u
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Text {
            anchors.fill: parent
            visible: root.title !== ""
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: root.title
            font.pixelSize: 3.4 * root.u
            font.strikeout: root.isDone
        }
        InkField {
            id: inkTitle
            // Handwritten title rendered at list size.
            anchors.fill: parent
            visible: root.title === ""
            editable: false
            u: root.u
            source: root.title === "" ? root.inkTitlePath : ""
        }
        TapHandler { onTapped: root.opened() }
    }

    Row {
        id: meta
        anchors.right: parent.right
        anchors.rightMargin: 2 * root.u
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2 * root.u

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: tasks.formatDuration(root.timeSpent)
                  + (root.timeEstimate > 0 ? " / " + tasks.formatDuration(root.timeEstimate) : "")
            font.pixelSize: 2.6 * root.u
            visible: root.timeSpent > 0 || root.timeEstimate > 0
        }
        EButton {
            u: root.u
            visible: !root.isDone
            implicitWidth: 8 * root.u
            text: root.isTracking ? "❚❚" : "▶"
            checked: root.isTracking
            onClicked: tasks.toggleTracking(root.taskId)
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
