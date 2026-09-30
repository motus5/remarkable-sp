import QtQuick

// Small toggle/label button for option rows (projects, tags, schedule, ...).
Rectangle {
    id: root
    property alias text: label.text
    property bool checked: false
    property real u: 10
    signal clicked()

    implicitWidth: label.implicitWidth + 3 * u
    implicitHeight: 5.5 * u
    radius: height / 2
    color: checked ? "black" : "white"
    border.color: "black"
    border.width: Math.max(2, u / 5)

    Text {
        id: label
        anchors.centerIn: parent
        font.pixelSize: 2.6 * root.u
        color: root.checked ? "white" : "black"
    }
    TapHandler { onTapped: root.clicked() }
}
