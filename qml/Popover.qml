import QtQuick
import RemarkableSP.Core

// reMarkable-style popup menu: white card, black frame, no shadow, no
// animation. Items: {text, icon?, checked?, detail?, action: function} or
// {separator: true} or {header: "…"}. Tapping outside closes it.
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 90
    property real u: 10
    property var items: []
    property string title
    property point at: Qt.point(0, 0)

    function open(menuItems, anchor, menuTitle) {
        items = menuItems
        title = menuTitle || ""
        const p = anchor.mapToItem(root, 0, 0)
        // Prefer right of the anchor (toolbar), else below it.
        let x = p.x + anchor.width + root.u
        let y = p.y
        if (x + card.width > root.width - root.u) {
            x = Math.max(root.u, Math.min(p.x, root.width - card.width - root.u))
            y = p.y + anchor.height + root.u
        }
        at = Qt.point(x, Math.max(root.u, Math.min(y, root.height - card.height - root.u)))
        visible = true
    }
    function close() { visible = false }

    // Swallow everything outside the card (finger, mouse and pen) and close.
    MouseArea { anchors.fill: parent; onClicked: root.close() }

    Rectangle {
        id: card
        x: root.at.x
        y: root.at.y
        width: Math.min(root.width - 2 * root.u, 56 * root.u)
        height: col.implicitHeight + 2 * root.u
        color: Theme.paper
        border.color: Theme.ink
        border.width: 3
        radius: root.u

        MouseArea { anchors.fill: parent } // swallow taps on the card background

        Column {
            id: col
            x: 0
            y: root.u
            width: parent.width

            Text {
                visible: root.title !== ""
                x: 3 * root.u
                height: 6 * root.u
                verticalAlignment: Text.AlignVCenter
                text: root.title
                font.pixelSize: 2.6 * root.u
                font.bold: true
                color: Theme.muted
            }

            Repeater {
                model: root.items
                Item {
                    width: col.width
                    height: modelData.separator ? 2 * root.u : (modelData.header ? 5.5 * root.u : 8 * root.u)

                    Rectangle {
                        visible: !!modelData.separator
                        anchors.verticalCenter: parent.verticalCenter
                        x: 2 * root.u
                        width: parent.width - 4 * root.u
                        height: 1
                        color: Theme.rule
                    }
                    Text {
                        visible: !!modelData.header
                        x: 3 * root.u
                        anchors.bottom: parent.bottom
                        text: modelData.header || ""
                        font.pixelSize: 2.3 * root.u
                        font.bold: true
                        color: Theme.muted
                    }
                    Item {
                        anchors.fill: parent
                        visible: !modelData.separator && !modelData.header
                        Rectangle {
                            anchors.fill: parent
                            anchors.leftMargin: root.u
                            anchors.rightMargin: root.u
                            radius: root.u * 0.6
                            color: modelData.checked ? Theme.selection : "transparent"
                        }
                        Icon {
                            id: ic
                            x: 3 * root.u
                            anchors.verticalCenter: parent.verticalCenter
                            width: 4.2 * root.u
                            height: width
                            name: modelData.icon || ""
                        }
                        Text {
                            anchors.left: ic.right
                            anchors.leftMargin: 2 * root.u
                            anchors.right: det.left
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                            text: modelData.text || ""
                            font.pixelSize: 3 * root.u
                            font.bold: !!modelData.checked
                        }
                        Text {
                            id: det
                            anchors.right: parent.right
                            anchors.rightMargin: 3 * root.u
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.detail || (modelData.checked ? "✓" : "")
                            font.pixelSize: 2.6 * root.u
                            color: Theme.muted
                        }
                        TapHandler {
                            onTapped: {
                                const keep = !!modelData.keepOpen
                                if (!keep)
                                    root.close()
                                if (modelData.action)
                                    modelData.action()
                            }
                        }
                    }
                }
            }
        }
    }
}
