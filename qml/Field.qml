import QtQuick
import RemarkableSP.Core

// Text field in reMarkable style (underlined), summons the on-screen keyboard.
Column {
    id: root
    property alias text: input.text
    property alias input: input
    property string label
    property string placeholder
    property bool password: false
    property real u: 10
    property real fontSize: 2.9 * u
    signal edited(string text)

    spacing: 0.6 * u
    Text {
        text: root.label
        font.pixelSize: 2.3 * root.u
        color: Theme.muted
        visible: root.label !== ""
    }
    Item {
        width: root.width
        height: root.fontSize * 2
        TextInput {
            id: input
            anchors.fill: parent
            anchors.rightMargin: 5 * root.u
            verticalAlignment: TextInput.AlignVCenter
            font.pixelSize: root.fontSize
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
                color: Theme.faint
                font.pixelSize: root.fontSize * 0.9
            }
        }
        Icon {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 3.6 * root.u
            height: width
            name: "keyboard"
            color: input.activeFocus ? Theme.ink : Theme.faint
            TapHandler { onTapped: input.forceActiveFocus() }
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: input.activeFocus ? 3 : 1.5
            color: input.activeFocus ? Theme.ink : Theme.faint
        }
    }
}
