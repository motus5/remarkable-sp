import QtQuick
import RemarkableSP.Core

// Writing area. Only pen (and mouse, for desktop development) draws; fingers
// are ignored here, which gives palm rejection for free. The pen's eraser end
// erases, or the pen tip erases while `eraserMode` is on.
Rectangle {
    id: root
    property alias source: canvas.source
    property alias canvas: canvas
    property bool editable: true
    property bool eraserMode: false
    property string placeholder: ""
    property real u: 10

    color: editable ? "white" : "transparent"
    border.color: editable ? "black" : "transparent"
    border.width: editable ? 2 : 0
    clip: true

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: root.u * 2
        visible: canvas.empty && root.editable
        text: root.placeholder
        color: "#888888"
        font.pixelSize: root.u * 3
        font.italic: true
    }

    InkCanvas {
        id: canvas
        anchors.fill: parent
        z: 1 // above ruled lines added by users of InkField
        penWidth: root.u * 0.35
    }

    // Autosave a moment after the last stroke.
    Timer {
        id: saveTimer
        interval: 1500
        onTriggered: canvas.save()
    }
    Connections {
        target: canvas
        function onModifiedChanged() { if (canvas.modified) saveTimer.restart() }
    }

    PointHandler {
        id: pen
        enabled: root.editable
        acceptedDevices: PointerDevice.Stylus | PointerDevice.Mouse
        acceptedPointerTypes: PointerDevice.Pen | PointerDevice.Generic | PointerDevice.Cursor
        function pressure() { return point.pressure > 0 ? point.pressure : 0.5 }
        onActiveChanged: {
            if (active) {
                if (root.eraserMode)
                    canvas.eraseAt(point.position.x, point.position.y, root.u * 1.5)
                else
                    canvas.beginStroke(point.position.x, point.position.y, pressure())
            } else if (!root.eraserMode) {
                canvas.endStroke()
            }
        }
        onPointChanged: {
            if (!active)
                return
            if (root.eraserMode)
                canvas.eraseAt(point.position.x, point.position.y, root.u * 1.5)
            else
                canvas.extendStroke(point.position.x, point.position.y, pressure())
        }
    }

    PointHandler {
        id: eraser
        enabled: root.editable
        acceptedDevices: PointerDevice.Stylus
        acceptedPointerTypes: PointerDevice.Eraser
        onPointChanged: if (active) canvas.eraseAt(point.position.x, point.position.y, root.u * 2)
    }
}
