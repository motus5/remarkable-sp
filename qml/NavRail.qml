import QtQuick
import RemarkableSP.Core

// Left navigation like the reMarkable library sidebar: an icon rail that
// expands (round button on top) into a panel with projects and tags.
Rectangle {
    id: root
    property real u: 10
    property bool expanded: false
    property string page: "tasks"
    property real railWidth: 12 * u
    signal navigate(string page)
    signal openMenu(var items, var anchor, string title)

    width: expanded ? 46 * u : railWidth
    color: Theme.paper
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.rule }

    function go(type, id) {
        app.openContext(type, id)
        root.navigate("tasks")
        root.expanded = false
    }
    readonly property bool todayActive: page === "tasks" && app.contextType === "TAG" && app.contextId === "TODAY"

    // ---- collapsed rail -------------------------------------------------
    Column {
        visible: !root.expanded
        width: root.railWidth
        y: 2 * root.u
        spacing: 1.5 * root.u

        Rectangle { // the round toggle known from the reMarkable toolbar
            anchors.horizontalCenter: parent.horizontalCenter
            width: 8 * root.u; height: width; radius: width / 2
            border.color: Theme.ink; border.width: 2
            Icon { anchors.centerIn: parent; width: 4.4 * root.u; height: width; name: "menu" }
            TapHandler { onTapped: root.expanded = true }
        }
        Item { width: 1; height: root.u }
        IconButton { u: root.u; size: 10 * root.u; icon: "today"; caption: "Heute"; checked: root.todayActive; onClicked: root.go("TAG", "TODAY") }
        IconButton {
            u: root.u; size: 10 * root.u; icon: "project"; caption: "Projekte"
            checked: root.page === "tasks" && app.contextType === "PROJECT"
            onClicked: root.expanded = true
        }
        IconButton {
            u: root.u; size: 10 * root.u; icon: "tag"; caption: "Tags"
            visible: app.tags.length > 0
            checked: root.page === "tasks" && app.contextType === "TAG" && app.contextId !== "TODAY"
            onClicked: root.expanded = true
        }
        IconButton { u: root.u; size: 10 * root.u; icon: "worklog"; caption: "Protokoll"; checked: root.page === "worklog"; onClicked: root.navigate("worklog") }
    }
    Column {
        visible: !root.expanded
        width: root.railWidth
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2 * root.u
        spacing: 1.5 * root.u
        IconButton {
            id: focusBtn
            u: root.u; size: 10 * root.u; icon: "focus"
            caption: app.focusActive ? app.focusRemaining + " min" : "Fokus"
            checked: app.focusActive
            onClicked: root.openMenu(root.focusMenu(), focusBtn, "Fokus-Timer")
        }
        IconButton {
            u: root.u; size: 10 * root.u; icon: "sync"
            visible: sync.configured
            caption: sync.busy ? "…" : (sync.lastOk ? "Sync" : "Fehler")
            badge: app.pendingCount > 0 ? String(app.pendingCount) : (sync.lastOk ? "" : "!")
            onClicked: sync.syncNow()
        }
        IconButton { u: root.u; size: 10 * root.u; icon: "settings"; caption: "Einstell."; checked: root.page === "settings"; onClicked: root.navigate("settings") }
    }

    function focusMenu() {
        if (app.focusActive)
            return [{ text: "Fokus beenden", icon: "close", action: () => app.stopFocus() }]
        const items = []
        for (const m of [15, 25, 45, 60, 90])
            items.push({ text: m + " Minuten", icon: "focus", checked: app.focusMinutes === m,
                         action: () => { app.focusMinutes = m; app.startFocus() } })
        return items
    }

    // ---- expanded panel ---------------------------------------------------
    Flickable {
        visible: root.expanded
        anchors.fill: parent
        anchors.rightMargin: 1
        contentHeight: panel.implicitHeight + 4 * root.u
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: panel
            width: parent.width
            y: 2 * root.u

            Item {
                width: parent.width
                height: 10 * root.u
                Rectangle {
                    x: 2 * root.u
                    width: 8 * root.u; height: width; radius: width / 2
                    border.color: Theme.ink; border.width: 2
                    Icon { anchors.centerIn: parent; width: 4.4 * root.u; height: width; name: "chevron-left" }
                    TapHandler { onTapped: root.expanded = false }
                }
                Text {
                    x: 12 * root.u
                    y: 1.5 * root.u
                    text: "Super Productivity"
                    font.pixelSize: 3.2 * root.u
                    font.bold: true
                }
            }

            component Entry: Item {
                id: entry
                property string icon
                property alias text: t.text
                property string detail
                property bool active: false
                property real indent: 0
                signal clicked()
                width: panel.width
                height: 8 * root.u
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: root.u
                    anchors.rightMargin: root.u
                    radius: root.u * 0.6
                    color: entry.active ? Theme.ink : "transparent"
                }
                Icon {
                    id: ei
                    x: 3 * root.u + entry.indent
                    anchors.verticalCenter: parent.verticalCenter
                    width: 4.2 * root.u; height: width
                    name: entry.icon
                    color: entry.active ? Theme.paper : Theme.ink
                }
                Text {
                    id: t
                    anchors.left: ei.right
                    anchors.leftMargin: 2 * root.u
                    anchors.right: d.left
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.pixelSize: 3 * root.u
                    color: entry.active ? Theme.paper : Theme.ink
                }
                Text {
                    id: d
                    anchors.right: parent.right
                    anchors.rightMargin: 3 * root.u
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.detail
                    font.pixelSize: 2.5 * root.u
                    color: entry.active ? Theme.paper : Theme.muted
                }
                TapHandler { onTapped: entry.clicked() }
            }
            component Heading: Text {
                x: 3 * root.u
                height: 7 * root.u
                verticalAlignment: Text.AlignBottom
                bottomPadding: root.u
                font.pixelSize: 2.3 * root.u
                font.bold: true
                font.letterSpacing: 0.15 * root.u
                color: Theme.muted
            }

            Entry { icon: "today"; text: "Heute"; detail: app.openCount; active: root.todayActive; onClicked: root.go("TAG", "TODAY") }
            Heading { text: "PROJEKTE" }
            Repeater {
                model: app.projects
                Entry {
                    icon: modelData.id === "INBOX_PROJECT" ? "inbox" : "project"
                    text: modelData.title
                    detail: modelData.count
                    active: root.page === "tasks" && app.contextType === "PROJECT" && app.contextId === modelData.id
                    onClicked: root.go("PROJECT", modelData.id)
                }
            }
            Heading { text: "TAGS"; visible: app.tags.length > 0 }
            Repeater {
                model: app.tags
                Entry {
                    icon: "tag"
                    text: modelData.title
                    detail: modelData.count
                    active: root.page === "tasks" && app.contextType === "TAG" && app.contextId === modelData.id
                    onClicked: root.go("TAG", modelData.id)
                }
            }
            Heading { text: "MEHR" }
            Entry { icon: "worklog"; text: "Arbeitsprotokoll"; active: root.page === "worklog"; onClicked: { root.navigate("worklog"); root.expanded = false } }
            Entry { icon: "export"; text: "Exportieren (Markdown)"; onClicked: { root.navigate("export"); root.expanded = false } }
            Entry { icon: "settings"; text: "Einstellungen"; active: root.page === "settings"; onClicked: { root.navigate("settings"); root.expanded = false } }
        }
    }
}
