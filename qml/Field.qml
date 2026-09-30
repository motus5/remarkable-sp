import QtQuick

// Labelled single-line text field that summons the on-screen keyboard.
Column {
    id: root
    property alias text: input.text
    property string label
    property string placeholder
    property bool password: false
    property real u: 10
    signal edited(string text)

    spacing: 0.5 * u
    Text { text: root.label; font.pixelSize: 2.4 * root.u; font.bold: true; visible: root.label !== "" }
    Rectangle {
        width: root.width
        height: 6 * root.u
        border.color: input.activeFocus ? "black" : "#999999"
        border.width: input.activeFocus ? 3 : 1
        TextInput {
            id: input
            anchors.fill: parent
            anchors.leftMargin: root.u
            anchors.rightMargin: root.u
            verticalAlignment: TextInput.AlignVCenter
            font.pixelSize: 2.8 * root.u
            clip: true
            echoMode: root.password && !activeFocus ? TextInput.Password : TextInput.Normal
            onActiveFocusChanged: {
                if (activeFocus)
                    Window.window.requestKeyboard(input)
                else
                    root.edited(text)
            }
            onAccepted: root.edited(text)
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !parent.text && !parent.activeFocus
                text: root.placeholder
                color: "#888888"
                font.pixelSize: 2.5 * root.u
            }
        }
    }
}
