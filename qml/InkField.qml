import QtQuick
import RemarkableSP.Core

// Writing surface. Only the pen (and mouse, for desktop development) draws;
// fingers pass through for paging and taps, which also gives palm rejection.
// The eraser end of the Marker erases, or the tip while `eraserMode` is on.
Item {
    id: root
    property alias source: canvas.source
    property alias canvas: canvas
    property alias paperTemplate: canvas.paperTemplate
    property alias lineSpacing: canvas.lineSpacing
    property bool editable: true
    property bool eraserMode: false
    property string placeholder: ""
    property real u: 10
    signal stroked()
    readonly property bool inputEnabled: editable && !(Window.window && Window.window.overlayOpen)

    clip: true

    Text {
        x: root.u
        anchors.verticalCenter: parent.verticalCenter
        visible: canvas.empty && root.editable && root.placeholder !== ""
        text: root.placeholder
        color: Theme.faint
        font.pixelSize: root.u * 3.2
    }

    InkCanvas {
        id: canvas
        anchors.fill: parent
        penWidth: root.u * [0.18, 0.3, 0.55][Math.max(0, Math.min(2, app.penWidth - 1))]
    }

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
        enabled: root.inputEnabled
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
                root.stroked()
            } else {
                root.stroked()
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
        enabled: root.inputEnabled
        acceptedDevices: PointerDevice.Stylus
        acceptedPointerTypes: PointerDevice.Eraser
        onPointChanged: if (active) canvas.eraseAt(point.position.x, point.position.y, root.u * 2)
        onActiveChanged: if (!active) root.stroked()
    }
}
