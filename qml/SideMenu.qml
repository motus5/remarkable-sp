import QtQuick

// Navigation like SP's side nav: Today, projects, tags, worklog, settings.
Rectangle {
    id: root
    property real u: 10
    signal navigate(string page)
    color: "white"
    border.color: "black"
    border.width: Math.max(3, u / 3)

    Flickable {
        anchors.fill: parent
        anchors.margins: 2 * root.u
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: col
            width: parent.width
            spacing: root.u

            component NavItem: Rectangle {
                id: item
                property alias text: t.text
                property string detail
                property bool active: false
                signal clicked()
                width: col.width
                height: 7 * root.u
                color: active ? "black" : "white"
                Text {
                    id: t
                    anchors.left: parent.left
                    anchors.leftMargin: root.u
                    anchors.right: d.left
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.pixelSize: 3.2 * root.u
                    color: item.active ? "white" : "black"
                }
                Text {
                    id: d
                    anchors.right: parent.right
                    anchors.rightMargin: root.u
                    anchors.verticalCenter: parent.verticalCenter
                    text: item.detail
                    font.pixelSize: 2.6 * root.u
                    color: item.active ? "white" : "#555555"
                }
                TapHandler { onTapped: item.clicked() }
            }
            component Section: Text {
                font.pixelSize: 2.4 * root.u
                font.bold: true
                color: "#555555"
                topPadding: root.u
            }

            NavItem {
                text: "☀ Heute"
                detail: app.openCount
                active: app.contextType === "TAG" && app.contextId === "TODAY"
                onClicked: { app.openContext("TAG", "TODAY"); root.navigate("tasks") }
            }
            Section { text: "PROJEKTE" }
            Repeater {
                model: app.projects
                NavItem {
                    text: modelData.title
                    detail: modelData.count
                    active: app.contextType === "PROJECT" && app.contextId === modelData.id
                    onClicked: { app.openContext("PROJECT", modelData.id); root.navigate("tasks") }
                }
            }
            Section { text: "TAGS"; visible: app.tags.length > 0 }
            Repeater {
                model: app.tags
                NavItem {
                    text: "# " + modelData.title
                    detail: modelData.count
                    active: app.contextType === "TAG" && app.contextId === modelData.id
                    onClicked: { app.openContext("TAG", modelData.id); root.navigate("tasks") }
                }
            }
            Section { text: "MEHR" }
            NavItem { text: "Arbeitsprotokoll"; onClicked: root.navigate("worklog") }
            NavItem { text: "Einstellungen, Sync & Updates"; onClicked: root.navigate("settings") }
            NavItem { text: "Markdown-Export"; onClicked: root.navigate("export") }
        }
    }
}
