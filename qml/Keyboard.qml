import QtQuick

// On-screen keyboard for devices without a hardware keyboard (reMarkable 2).
// Types into `target` (any TextInput). German layout, no animations.
Rectangle {
    id: root
    property var target: null
    property real u: 10
    property bool shift: false
    property bool symbols: false
    signal done()

    color: "white"
    height: col.implicitHeight + 3 * u
    Rectangle { width: parent.width; height: Math.max(2, root.u / 4); color: "black" }

    readonly property var letters: [
        "1234567890ß", "qwertzuiopü", "asdfghjklöä", "yxcvbnm.-@"
    ]
    readonly property var symbolRows: [
        "1234567890?", "!\"§$%&/()=+", "#'*~<>|\\{}[", ":;,_^°€&@/"
    ]

    function type(ch) {
        if (!target)
            return
        const t = shift && !symbols ? ch.toUpperCase() : ch
        if (target.selectedText.length > 0)
            target.remove(target.selectionStart, target.selectionEnd)
        target.insert(target.cursorPosition, t)
        if (shift)
            shift = false
    }
    function backspace() {
        if (!target)
            return
        if (target.selectedText.length > 0)
            target.remove(target.selectionStart, target.selectionEnd)
        else if (target.cursorPosition > 0)
            target.remove(target.cursorPosition - 1, target.cursorPosition)
    }

    Column {
        id: col
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 1.5 * root.u
        spacing: 0.8 * root.u

        Repeater {
            model: root.symbols ? root.symbolRows : root.letters
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 0.6 * root.u
                Repeater {
                    model: modelData.split("")
                    Key {
                        text: root.shift && !root.symbols ? modelData.toUpperCase() : modelData
                        onClicked: root.type(modelData)
                    }
                }
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0.6 * root.u
            Key { text: "⇧"; wide: 1.5; checked: root.shift; onClicked: root.shift = !root.shift }
            Key { text: root.symbols ? "abc" : "#+="; wide: 1.5; onClicked: root.symbols = !root.symbols }
            Key { text: "Leer"; wide: 4; onClicked: root.type(" ") }
            Key { text: "⌫"; wide: 1.5; onClicked: root.backspace() }
            Key { text: "Fertig"; wide: 2; checked: true; onClicked: root.done() }
        }
    }

    component Key: Rectangle {
        id: key
        property alias text: t.text
        property real wide: 1
        property bool checked: false
        signal clicked()
        width: (7.4 * root.u) * wide + (wide - 1) * 0.6 * root.u
        height: 7 * root.u
        radius: root.u * 0.6
        color: checked ? "black" : "white"
        border.color: "black"
        border.width: 2
        Text {
            id: t
            anchors.centerIn: parent
            font.pixelSize: 3.2 * root.u
            color: key.checked ? "white" : "black"
        }
        TapHandler { onTapped: key.clicked() }
    }
}
