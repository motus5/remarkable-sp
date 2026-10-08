import QtQuick
import RemarkableSP.Core

// Toolbar/rail button: line icon, optional caption, selected = black tile.
Item {
    id: root
    property string icon
    property string caption
    property string badge
    property bool checked: false
    property bool enabled: true
    property real u: 10
    property real size: 10 * u
    signal clicked()
    signal pressAndHold()

    implicitWidth: size
    implicitHeight: size + (caption !== "" ? 3 * u : 0)
    opacity: enabled ? 1 : 0.35

    Rectangle {
        id: tile
        width: root.size * 0.8
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        y: (root.size - height) / 2
        radius: root.u
        color: root.checked ? Theme.ink : "transparent"
        Icon {
            anchors.centerIn: parent
            width: root.size * 0.5
            height: width
            name: root.icon
            color: root.checked ? Theme.paper : Theme.ink
        }
    }
    Rectangle {
        visible: root.badge !== ""
        anchors.right: tile.right
        anchors.top: tile.top
        anchors.margins: -0.6 * root.u
        width: Math.max(height, badgeText.implicitWidth + 1.2 * root.u)
        height: 3.4 * root.u
        radius: height / 2
        color: Theme.ink
        Text {
            id: badgeText
            anchors.centerIn: parent
            text: root.badge
            color: Theme.paper
            font.pixelSize: 2.1 * root.u
            font.bold: true
        }
    }
    Text {
        visible: root.caption !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.size - 0.6 * root.u
        text: root.caption
        font.pixelSize: 2.1 * root.u
        color: Theme.ink
    }
    TapHandler {
        enabled: root.enabled
        onTapped: root.clicked()
        onLongPressed: root.pressAndHold()
    }
}
