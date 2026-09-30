import QtQuick
import RemarkableSP.Core

// Worklog: tracked time per day and task (SP's worklog / daily summary).
Item {
    id: root
    property real u: 10
    property var days: app.worklog(30)

    Connections {
        target: app
        function onDataChanged() { root.days = app.worklog(30) }
    }

    TopBar {
        id: bar
        width: parent.width
        u: root.u
        title: "Arbeitsprotokoll"
        subtitle: "Letzte 30 Tage"
    }

    ListView {
        id: list
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 4 * root.u
        anchors.rightMargin: 4 * root.u
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.days
        spacing: 3 * root.u
        delegate: Column {
            width: list.width
            spacing: 0.8 * root.u
            Item {
                width: parent.width
                height: 5 * root.u
                Text { text: app.formatDay(modelData.day); font.pixelSize: 3.2 * root.u; font.weight: Font.DemiBold }
                Text { anchors.right: parent.right; text: app.formatDuration(modelData.total); font.pixelSize: 3.2 * root.u; font.weight: Font.DemiBold }
            }
            Rectangle { width: parent.width; height: 2; color: Theme.ink }
            Repeater {
                model: modelData.tasks
                Item {
                    width: list.width
                    height: 5 * root.u
                    Text { width: parent.width * 0.8; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight; text: modelData.title; font.pixelSize: 2.7 * root.u }
                    Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: app.formatDuration(modelData.ms); font.pixelSize: 2.7 * root.u; color: Theme.muted }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.rule }
                }
            }
        }
        Column {
            anchors.centerIn: parent
            visible: list.count === 0
            spacing: 2 * root.u
            Icon { anchors.horizontalCenter: parent.horizontalCenter; width: 12 * root.u; height: width; name: "worklog"; color: Theme.faint }
            Text { text: "Noch keine erfasste Zeit"; font.pixelSize: 3.4 * root.u; color: Theme.muted }
        }
    }
}
