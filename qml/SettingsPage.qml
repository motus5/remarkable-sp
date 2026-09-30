import QtQuick
import RemarkableSP.Core

// Settings in the manner of the reMarkable settings screens: grouped
// sections with quiet headings and hairline rules.
Item {
    id: root
    property real u: 10

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 50 * root.u // room for the keyboard
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: col
            x: 4 * root.u
            width: parent.width - 8 * root.u
            spacing: 2 * root.u

            TopBar { u: root.u; width: parent.width + 8 * root.u; x: -4 * root.u; title: "Einstellungen"; subtitle: "Version " + updater.currentVersion }

            component Section: Column {
                property alias title: h.text
                width: col.width
                spacing: 1.6 * root.u
                Text { id: h; font.pixelSize: 3.4 * root.u; font.weight: Font.DemiBold; topPadding: 2 * root.u }
                Rectangle { width: parent.width; height: 1; color: Theme.rule }
            }
            component Hint: Text {
                width: col.width
                wrapMode: Text.Wrap
                font.pixelSize: 2.4 * root.u
                color: Theme.muted
            }
            component Option: Item {
                id: opt
                property alias text: ot.text
                property bool checked
                signal clicked()
                width: col.width
                height: 7 * root.u
                Rectangle {
                    x: 0.5 * root.u
                    anchors.verticalCenter: parent.verticalCenter
                    width: 4.4 * root.u; height: width; radius: width / 2
                    border.color: Theme.ink; border.width: 2
                    Rectangle { anchors.centerIn: parent; width: parent.width * 0.5; height: width; radius: width / 2; color: Theme.ink; visible: opt.checked }
                }
                Text { id: ot; x: 7 * root.u; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 3 * root.u }
                TapHandler { onTapped: opt.clicked() }
            }

            Section { title: "Sync mit Super Productivity" }
            Hint {
                text: "Nutzt denselben Sync-Ordner wie Super Productivity auf Computer und Handy. "
                      + "Richte den Sync zuerst dort ein und synchronisiere einmal. „Surgical sync“ muss in SP aus sein."
            }
            Option { text: "Aus"; checked: sync.provider === ""; onClicked: sync.provider = "" }
            Option { text: "WebDAV / Nextcloud"; checked: sync.provider === "webdav"; onClicked: sync.provider = "webdav" }
            Option { text: "Ordner auf dem Gerät"; checked: sync.provider === "folder"; onClicked: sync.provider = "folder" }

            Column {
                width: parent.width
                spacing: 2 * root.u
                visible: sync.provider === "webdav"
                Field {
                    width: parent.width; u: root.u
                    label: "WebDAV-Adresse"
                    placeholder: "https://cloud.example.de/remote.php/dav/files/NAME/"
                    text: sync.webdavUrl
                    onEdited: (t) => sync.webdavUrl = t.trim()
                }
                Hint { text: "Wie in SP: Basis-Adresse ohne Sync-Ordner. Nextcloud: …/remote.php/dav/files/<Benutzer>/" }
                Field { width: parent.width; u: root.u; label: "Benutzer"; text: sync.webdavUser; onEdited: (t) => sync.webdavUser = t }
                Field { width: parent.width; u: root.u; label: "Passwort / App-Passwort"; password: true; text: sync.webdavPassword; onEdited: (t) => sync.webdavPassword = t }
                Field { width: parent.width; u: root.u; label: "Sync-Ordner"; placeholder: "super-productivity"; text: sync.syncFolder; onEdited: (t) => sync.syncFolder = t.trim() }
            }
            Column {
                width: parent.width
                spacing: 2 * root.u
                visible: sync.provider === "folder"
                Field { width: parent.width; u: root.u; label: "Ordner"; placeholder: "/home/root/sp-sync"; text: sync.localPath; onEdited: (t) => sync.localPath = t.trim() }
                Hint { text: "Ordner, den SPs „Lokale Datei“-Sync nutzt, z. B. per Syncthing oder rclone aufs Gerät gespiegelt." }
            }
            Field {
                width: parent.width; u: root.u
                visible: sync.provider !== ""
                label: "Verschlüsselungs-Passwort (falls in SP aktiviert)"
                password: true
                text: sync.encryptKey
                onEdited: (t) => sync.encryptKey = t
            }
            Hint { visible: sync.provider !== "" && !sync.encryptionSupported; text: "Diese Version wurde ohne Verschlüsselung gebaut." }

            Text { visible: sync.provider !== ""; text: "Automatisch synchronisieren"; font.pixelSize: 2.3 * root.u; color: Theme.muted }
            Flow {
                width: parent.width
                spacing: 1.2 * root.u
                visible: sync.provider !== ""
                Repeater {
                    model: [[0, "Manuell"], [5, "Alle 5 min"], [10, "Alle 10 min"], [30, "Alle 30 min"]]
                    Chip { u: root.u; text: modelData[1]; checked: sync.intervalMinutes === modelData[0]; onClicked: sync.intervalMinutes = modelData[0] }
                }
            }
            Row {
                spacing: 3 * root.u
                visible: sync.provider !== ""
                RmButton { u: root.u; primary: true; icon: "sync"; text: sync.busy ? "Synchronisiere …" : "Jetzt synchronisieren"; enabled: sync.configured && !sync.busy; onClicked: sync.syncNow() }
            }
            Hint {
                visible: sync.provider !== ""
                text: (sync.status !== "" ? (sync.lastOk ? "" : "⚠ ") + sync.status + "\n" : "")
                      + "Letzter Sync: " + sync.lastSyncText + "  ·  ausstehende Änderungen: " + app.pendingCount
            }

            Section { title: "Schreiben" }
            Text { text: "Stiftbreite"; font.pixelSize: 2.3 * root.u; color: Theme.muted }
            Flow {
                width: parent.width
                spacing: 1.2 * root.u
                Chip { u: root.u; icon: "pen-fine"; text: "Fein"; checked: app.penWidth === 1; onClicked: app.penWidth = 1 }
                Chip { u: root.u; icon: "pen"; text: "Mittel"; checked: app.penWidth === 2; onClicked: app.penWidth = 2 }
                Chip { u: root.u; icon: "pen-thick"; text: "Breit"; checked: app.penWidth === 3; onClicked: app.penWidth = 3 }
            }
            Text { text: "Vorlage für Notizen"; font.pixelSize: 2.3 * root.u; color: Theme.muted }
            Flow {
                width: parent.width
                spacing: 1.2 * root.u
                Repeater {
                    model: [["lined", "Liniert"], ["grid", "Kariert"], ["dots", "Punkte"], ["blank", "Leer"]]
                    Chip { u: root.u; icon: "template"; text: modelData[1]; checked: app.paperTemplate === modelData[0]; onClicked: app.paperTemplate = modelData[0] }
                }
            }

            Section { title: "Software-Update" }
            Hint { text: "Installiert: Version " + updater.currentVersion + (updater.status !== "" ? "\n" + updater.status : "") }
            Row {
                spacing: 3 * root.u
                RmButton { u: root.u; icon: "update"; text: "Nach Updates suchen"; enabled: !updater.busy; onClicked: updater.check() }
                RmButton { u: root.u; visible: updater.updateAvailable; primary: true; text: "Installieren"; enabled: !updater.busy; onClicked: updater.install() }
                RmButton { u: root.u; visible: updater.readyToRestart; primary: true; text: "Neu starten"; onClicked: { app.stopTracking(); updater.restart() } }
            }

            Section { title: "Daten" }
            Hint { text: "Datenordner: " + app.dataDir + "\nHandschrift bleibt auf dem Gerät. Aufgaben, Zeiten und Planung werden synchronisiert." }
        }
    }
}
