import QtQuick

// Flat, high-contrast button without animations (e-ink friendly).
// Reacts to finger, mouse and pen taps.
Rectangle {
    id: root
    property alias text: label.text
    property bool checked: false
    property bool enabled: true
    property real u: 10
    signal clicked()

    implicitWidth: Math.max(label.implicitWidth + 2.5 * u, 7 * u)
    implicitHeight: 7 * u
    radius: u
    color: checked ? "black" : "white"
    border.color: enabled ? "black" : "#999999"
    border.width: Math.max(2, u / 4)

    Text {
        id: label
        anchors.centerIn: parent
        font.pixelSize: 2.8 * root.u
        font.bold: true
        color: root.checked ? "white" : (root.enabled ? "black" : "#999999")
    }

    TapHandler {
        enabled: root.enabled
        onTapped: root.clicked()
    }
}
