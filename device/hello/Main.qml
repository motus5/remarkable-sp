import QtQuick
import QtQuick.Window

// Shows a black frame, a text and toggles a box on every touch, so display
// refresh and touch input can both be checked by eye.
Window {
    width: Screen.width
    height: Screen.height
    visible: true
    color: "white"

    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        color: "transparent"
        border.color: "black"
        border.width: 8
    }
    Text {
        x: 60; y: 60
        font.pixelSize: 64
        text: "Hello reMarkable 1"
    }
    Text {
        x: 60; y: 160
        font.pixelSize: 36
        text: "Bildschirm " + Screen.width + " × " + Screen.height + " – zum Testen tippen"
    }
    Rectangle {
        id: box
        x: 60; y: 260
        width: 400; height: 400
        color: "black"
    }
    MouseArea {
        anchors.fill: parent
        onPressed: box.visible = !box.visible
    }
}
