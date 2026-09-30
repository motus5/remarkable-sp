import QtQuick

// Worklog: tracked time per day and task (like SP's worklog / daily summary).
Rectangle {
    id: root
    property real u: 10
    property var days: app.worklog(30)
    signal closed()
    color: "white"

    Connections {
        target: app
        function onDataChanged() { root.days = app.worklog(30) }
    }

    EButton {
        id: back
        x: 2 * root.u; y: 2 * root.u
        u: root.u
        text: "‹ Zurück"
        onClicked: root.closed()
    }
    Text {
        anchors.left: back.right
        anchors.leftMargin: 2 * root.u
        anchors.verticalCenter: back.verticalCenter
        text: "Arbeitsprotokoll"
        font.pixelSize: 4.2 * root.u
        font.bold: true
    }

    ListView {
        id: list
        anchors.top: back.bottom
        anchors.topMargin: 2 * root.u
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2 * root.u
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.days
        spacing: 2 * root.u
        delegate: Column {
            width: list.width
            spacing: 0.5 * root.u
            Row {
                width: parent.width
                Text { width: parent.width * 0.7; text: app.formatDay(modelData.day) + "  (" + modelData.day + ")"; font.pixelSize: 3 * root.u; font.bold: true }
                Text { width: parent.width * 0.3; horizontalAlignment: Text.AlignRight; text: app.formatDuration(modelData.total); font.pixelSize: 3 * root.u; font.bold: true }
            }
            Rectangle { width: parent.width; height: 2; color: "black" }
            Repeater {
                model: modelData.tasks
                Row {
                    width: list.width
                    Text { width: parent.width * 0.8; elide: Text.ElideRight; text: modelData.title; font.pixelSize: 2.6 * root.u }
                    Text { width: parent.width * 0.2; horizontalAlignment: Text.AlignRight; text: app.formatDuration(modelData.ms); font.pixelSize: 2.6 * root.u }
                }
            }
        }
        Text {
            anchors.centerIn: parent
            visible: list.count === 0
            text: "Noch keine erfasste Zeit."
            font.pixelSize: 3.2 * root.u
            color: "#666666"
        }
    }
}
