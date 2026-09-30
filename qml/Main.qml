import QtQuick

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
    property string page: "tasks" // tasks | worklog | settings
    property string openTaskId: ""
    property bool menuOpen: false
    property var keyboardTarget: null

    function requestKeyboard(input) {
        if (rmOsk)
            keyboardTarget = input
    }
    function show(text) { banner.show(text) }

    Item {
        id: content
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: keyboard.visible ? keyboard.top : parent.bottom

        Header {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            u: win.u
            visible: win.page === "tasks" && win.openTaskId === ""
            onMenuRequested: win.menuOpen = true
        }

        TaskList {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            u: win.u
            visible: win.page === "tasks" && win.openTaskId === ""
            onOpenTask: (id) => win.openTaskId = id
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

        Loader {
            anchors.fill: parent
            active: win.page === "worklog"
            sourceComponent: WorklogPage { u: win.u; onClosed: win.page = "tasks" }
        }

        Loader {
            anchors.fill: parent
            active: win.page === "settings"
            sourceComponent: SettingsPage { u: win.u; onClosed: win.page = "tasks" }
        }
    }

    // Side navigation as an overlay; tapping outside closes it.
    Rectangle {
        anchors.fill: parent
        visible: win.menuOpen
        color: "#80ffffff"
        z: 50
        TapHandler { onTapped: win.menuOpen = false }
        SideMenu {
            width: parent.width * 0.72
            height: parent.height
            u: win.u
            onNavigate: (p) => {
                win.menuOpen = false
                win.openTaskId = ""
                if (p === "export")
                    banner.show(app.exportNow())
                else
                    win.page = p
            }
        }
    }

    Keyboard {
        id: keyboard
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        u: win.u
        z: 60
        target: win.keyboardTarget
        visible: win.keyboardTarget !== null && win.keyboardTarget.activeFocus
        onDone: {
            const t = win.keyboardTarget
            win.keyboardTarget = null
            if (t) {
                t.accepted()
                t.focus = false
            }
        }
    }

    Connections {
        target: app
        function onFocusFinished() { banner.show("Fokus-Session vorbei – kurze Pause!") }
    }
    Connections {
        target: sync
        function onRemoteNewer() { banner.show("Super Productivity nutzt ein neueres Datenformat.\nBitte unter Einstellungen nach Updates suchen.") }
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
