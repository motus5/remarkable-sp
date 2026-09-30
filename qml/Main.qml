import QtQuick
import QtQuick.Window

Window {
    id: win
    // reMarkable 2 is 1404x1872; on desktop we open at half size.
    width: 702
    height: 936
    visible: true
    visibility: rmFullscreen ? Window.FullScreen : Window.Windowed
    title: "reMarkable SP"
    color: "white"

    // Layout unit: 1u = 1% of the screen width (14px on a reMarkable 2).
    readonly property real u: width / 100
    property string openTaskId: ""

    Header {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        u: win.u
        visible: win.openTaskId === ""
    }

    TaskList {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        u: win.u
        visible: win.openTaskId === ""
        onOpenTask: (id) => win.openTaskId = id
        onMessage: (text) => banner.show(text)
    }

    Loader {
        anchors.fill: parent
        active: win.openTaskId !== ""
        sourceComponent: TaskDetail {
            u: win.u
            taskId: win.openTaskId
            onTaskIdChanged: win.openTaskId = taskId
            onClosed: win.openTaskId = ""
        }
    }

    Connections {
        target: tasks
        function onFocusFinished() { banner.show("Fokus-Session vorbei – kurze Pause!") }
    }

    Rectangle {
        id: banner
        property alias text: bannerText.text
        function show(t) { text = t; visible = true }
        visible: false
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: bannerText.implicitHeight + 14 * win.u
        color: "white"
        border.color: "black"
        border.width: Math.max(3, win.u / 2)
        radius: win.u
        z: 100

        Text {
            id: bannerText
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 3 * win.u
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 3.2 * win.u
        }
        EButton {
            u: win.u
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 2 * win.u
            text: "OK"
            onClicked: banner.visible = false
        }
    }
}
