import QtQuick
import RemarkableSP.Core

// Text button as in reMarkable dialogs: outlined, primary = filled black.
Rectangle {
    id: root
    property alias text: label.text
    property string icon
    property bool primary: false
    property bool enabled: true
    property real u: 10
    signal clicked()

    implicitWidth: row.implicitWidth + 5 * u
    implicitHeight: 7.5 * u
    radius: 0.8 * u
    color: primary ? Theme.ink : Theme.paper
    border.color: Theme.ink
    border.width: 2
    opacity: enabled ? 1 : 0.35

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 1.2 * root.u
        Icon {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            width: 4 * root.u
            height: width
            name: root.icon
            color: root.primary ? Theme.paper : Theme.ink
        }
        Text {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 2.8 * root.u
            font.family: Theme.font
            color: root.primary ? Theme.paper : Theme.ink
        }
    }
    TapHandler { enabled: root.enabled; onTapped: root.clicked() }
}
