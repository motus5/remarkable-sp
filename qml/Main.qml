import QtQuick
import RemarkableSP.Core

Window {
    id: win
    // reMarkable 2 is 1404x1872; on desktop we open at half size.
    width: 702
    height: 936
    visible: true
    visibility: rmFullscreen ? Window.FullScreen : Window.Windowed
    title: "reMarkable SP"
    color: Theme.paper

    // Layout unit: 1u = 1% of the screen width (14px on a reMarkable 2).
    readonly property real u: width / 100
    property string page: "tasks" // tasks | worklog | settings
    property string openTaskId: ""
    property var keyboardTarget: null
    // While a menu or dialog is open the pen must not write on the page below.
    readonly property bool overlayOpen: popover.visible || dialog.visible

    function requestKeyboard(input) {
        if (rmOsk)
            keyboardTarget = input
    }
    function openMenu(items, anchor, title) { popover.open(items, anchor, title) }

    Item {
        id: content
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: keyboard.visible ? keyboard.top : parent.bottom

        NavRail {
            id: rail
            height: win.height // keeps its layout; the keyboard slides over it
            u: win.u
            z: 5
            page: win.page
            visible: win.openTaskId === ""
            onNavigate: (p) => {
                if (p === "export")
                    toast.show(app.exportNow())
                else
                    win.page = p
            }
            onOpenMenu: (items, anchor, title) => win.openMenu(items, anchor, title)
        }

        Item {
            id: main
            anchors.left: parent.left
            anchors.leftMargin: rail.railWidth
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: win.openTaskId === ""

            TopBar {
                id: top
                width: parent.width
                u: win.u
                visible: win.page === "tasks"
                title: app.contextTitle
                subtitle: {
                    const s = [app.formatDuration(app.todayTotal) + " heute erfasst"]
                    if (app.contextType === "TAG" && app.contextId === "TODAY") {
                        s.unshift(app.openCount + " offen")
                        if (app.todayEstimate > 0) s.push("noch ~" + app.formatDuration(app.todayEstimate))
                    }
                    return s.join("  ·  ")
                }
                showTabs: true
                showAdd: true
                onAdd: win.openTaskId = app.addTask("")
            }
            TaskList {
                anchors.top: top.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                u: win.u
                visible: win.page === "tasks"
                onOpenTask: (id) => win.openTaskId = id
                onOpenMenu: (items, anchor, title) => win.openMenu(items, anchor, title)
            }
            Loader {
                anchors.fill: parent
                active: win.page === "worklog"
                sourceComponent: WorklogPage { u: win.u }
            }
            Loader {
                anchors.fill: parent
                active: win.page === "settings"
                sourceComponent: SettingsPage { u: win.u }
            }
        }

        // Tapping the dimmed area closes the expanded navigation.
        Item {
            anchors.fill: parent
            visible: rail.expanded
            z: 4
            TapHandler { onTapped: rail.expanded = false }
        }

        Loader {
            anchors.fill: parent
            active: win.openTaskId !== ""
            z: 6
            sourceComponent: TaskDetail {
                u: win.u
                taskId: win.openTaskId
                onTaskIdChanged: win.openTaskId = taskId
                onClosed: win.openTaskId = ""
                onOpenMenu: (items, anchor, title) => win.openMenu(items, anchor, title)
            }
        }
    }

    Popover { id: popover; u: win.u }

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
        function onFocusFinished() { dialog.show("Fokus-Session beendet", "Zeit für eine kurze Pause.") }
    }
    Connections {
        target: sync
        function onRemoteNewer() { dialog.show("Update empfohlen", "Super Productivity nutzt ein neueres Datenformat. Bitte unter Einstellungen nach Updates suchen.") }
    }

    // Small notice at the bottom, like the reMarkable "toast".
    Rectangle {
        id: toast
        function show(t) { toastText.text = t; visible = true; toastTimer.restart() }
        visible: false
        z: 95
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6 * win.u
        width: Math.min(parent.width - 8 * win.u, toastText.implicitWidth + 6 * win.u)
        height: toastText.implicitHeight + 3 * win.u
        radius: win.u
        color: Theme.ink
        Text {
            id: toastText
            anchors.centerIn: parent
            width: Math.min(implicitWidth, win.width - 14 * win.u)
            wrapMode: Text.Wrap
            color: Theme.paper
            font.pixelSize: 2.6 * win.u
        }
        Timer { id: toastTimer; interval: 5000; onTriggered: toast.visible = false }
        TapHandler { onTapped: toast.visible = false }
    }

    // Modal dialog in reMarkable style: title, text, one button.
    Item {
        id: dialog
        function show(t, body) { dTitle.text = t; dBody.text = body; visible = true }
        anchors.fill: parent
        visible: false
        z: 100
        Rectangle { anchors.fill: parent; color: "#99ffffff" }
        MouseArea { anchors.fill: parent }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.78
            height: dcol.implicitHeight + 8 * win.u
            color: Theme.paper
            border.color: Theme.ink
            border.width: 3
            radius: win.u
            Column {
                id: dcol
                anchors.centerIn: parent
                width: parent.width - 10 * win.u
                spacing: 3 * win.u
                Text { id: dTitle; width: parent.width; font.pixelSize: 3.8 * win.u; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                Text { id: dBody; width: parent.width; font.pixelSize: 2.8 * win.u; wrapMode: Text.Wrap; color: Theme.muted }
                RmButton { u: win.u; anchors.right: parent.right; primary: true; text: "OK"; onClicked: dialog.visible = false }
            }
        }
    }
}
