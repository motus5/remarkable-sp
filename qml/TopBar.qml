import QtQuick
import RemarkableSP.Core

// Page header like the reMarkable library: large title, quiet subtitle,
// actions on the right. Shows the running timer as a slim status line.
Item {
    id: root
    property real u: 10
    property string title
    property string subtitle
    property bool showTabs: false
    property bool showAdd: false
    signal add()

    implicitHeight: col.implicitHeight + 3 * u

    Column {
        id: col
        x: 4 * root.u
        y: 2.5 * root.u
        width: parent.width - 8 * root.u
        spacing: 1.2 * root.u

        Item {
            width: parent.width
            height: 9 * root.u
            Text {
                anchors.left: parent.left
                anchors.right: addBtn.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                elide: Text.ElideRight
                font.pixelSize: 5.4 * root.u
                font.weight: Font.DemiBold
            }
            Rectangle {
                id: addBtn
                visible: root.showAdd
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: visible ? 9 * root.u : 0
                height: 9 * root.u
                radius: width / 2
                color: Theme.ink
                Icon { anchors.centerIn: parent; width: 5 * root.u; height: width; name: "plus"; color: Theme.paper }
                TapHandler { onTapped: root.add() }
            }
        }

        Row {
            width: parent.width
            spacing: 4 * root.u
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.subtitle
                font.pixelSize: 2.6 * root.u
                color: Theme.muted
            }
        }

        Row {
            visible: root.showTabs
            spacing: 4 * root.u
            component Tab: Item {
                id: tab
                property alias text: tt.text
                property bool active
                signal clicked()
                width: tt.implicitWidth
                height: 6 * root.u
                Text {
                    id: tt
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 3 * root.u
                    font.bold: tab.active
                    color: tab.active ? Theme.ink : Theme.muted
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 3; color: Theme.ink; visible: tab.active }
                TapHandler { onTapped: tab.clicked() }
            }
            Tab { text: "Offen"; active: !app.showDone; onClicked: app.showDone = false }
            Tab { text: "Erledigt"; active: app.showDone; onClicked: app.showDone = true }
        }

        // Running timer, like a status line.
        Rectangle {
            visible: app.currentTaskId !== ""
            width: parent.width
            height: 7 * root.u
            radius: root.u
            color: Theme.ink
            Icon {
                id: ti
                x: 2 * root.u
                anchors.verticalCenter: parent.verticalCenter
                width: 4 * root.u; height: width
                name: "focus"
                color: Theme.paper
            }
            Text {
                anchors.left: ti.right
                anchors.leftMargin: 1.5 * root.u
                anchors.right: stop.left
                anchors.verticalCenter: parent.verticalCenter
                text: app.currentTaskTitle
                elide: Text.ElideRight
                font.pixelSize: 2.8 * root.u
                color: Theme.paper
            }
            Rectangle {
                id: stop
                anchors.right: parent.right
                anchors.rightMargin: root.u
                anchors.verticalCenter: parent.verticalCenter
                width: 5.4 * root.u; height: width; radius: width / 2
                color: Theme.paper
                Icon { anchors.centerIn: parent; width: 3.2 * root.u; height: width; name: "pause" }
                TapHandler { onTapped: app.stopTracking() }
            }
        }
    }
}
