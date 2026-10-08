import QtQuick
import RemarkableSP.Core

// Compact property chip ("📅 Morgen", "30 m", "# Tag"): tap to change.
Rectangle {
    id: root
    property alias text: label.text
    property string icon
    property bool checked: false
    property real u: 10
    signal clicked()

    implicitWidth: row.implicitWidth + 3 * u
    implicitHeight: 5.6 * u
    radius: height / 2
    color: checked ? Theme.ink : Theme.paper
    border.color: Theme.ink
    border.width: checked ? 0 : 1.5

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 0.8 * root.u
        Icon {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            width: 3.2 * root.u
            height: width
            name: root.icon
            color: root.checked ? Theme.paper : Theme.ink
        }
        Text {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 2.5 * root.u
            color: root.checked ? Theme.paper : Theme.ink
        }
    }
    TapHandler { onTapped: root.clicked() }
}
