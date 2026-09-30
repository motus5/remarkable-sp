import QtQuick

// Sync (WebDAV / Nextcloud / folder), updates, focus timer.
Rectangle {
    id: root
    property real u: 10
    signal closed()
    color: "white"

    Flickable {
        anchors.fill: parent
        anchors.margins: 2 * root.u
        contentHeight: col.implicitHeight + 40 * root.u // room for the keyboard
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: col
            width: parent.width
            spacing: 1.5 * root.u

            component Heading: Text {
                font.pixelSize: 3.6 * root.u
                font.bold: true
                topPadding: root.u
            }
            component Hint: Text {
                width: col.width
                wrapMode: Text.Wrap
                font.pixelSize: 2.3 * root.u
                color: "#444444"
            }

            EButton { u: root.u; text: "‹ Zurück"; onClicked: root.closed() }

            Heading { text: "Sync mit Super Productivity" }
            Hint {
                text: "Nutzt denselben Sync-Ordner wie Super Productivity (Desktop/Handy). "
                      + "Richte den Sync zuerst dort ein (WebDAV/Nextcloud oder lokaler Ordner) und synchronisiere einmal. "
                      + "„Surgical sync“ (geteilte Dateien) muss in SP aus sein."
            }
            Flow {
                width: parent.width
                spacing: root.u
                Chip { u: root.u; text: "Aus"; checked: sync.provider === ""; onClicked: sync.provider = "" }
                Chip { u: root.u; text: "WebDAV / Nextcloud"; checked: sync.provider === "webdav"; onClicked: sync.provider = "webdav" }
                Chip { u: root.u; text: "Ordner"; checked: sync.provider === "folder"; onClicked: sync.provider = "folder" }
            }

            Column {
                width: parent.width
                spacing: root.u
                visible: sync.provider === "webdav"
                Field {
                    width: parent.width; u: root.u
                    label: "WebDAV-Adresse"
                    placeholder: "https://cloud.example.de/remote.php/dav/files/NAME/"
                    text: sync.webdavUrl
                    onEdited: (t) => sync.webdavUrl = t.trim()
                }
                Hint { text: "Wie in SP: Basis-URL ohne den Sync-Ordner. Nextcloud: …/remote.php/dav/files/<Benutzer>/" }
                Field { width: parent.width; u: root.u; label: "Benutzer"; text: sync.webdavUser; onEdited: (t) => sync.webdavUser = t }
                Field { width: parent.width; u: root.u; label: "Passwort / App-Passwort"; password: true; text: sync.webdavPassword; onEdited: (t) => sync.webdavPassword = t }
                Field { width: parent.width; u: root.u; label: "Sync-Ordner"; placeholder: "super-productivity"; text: sync.syncFolder; onEdited: (t) => sync.syncFolder = t.trim() }
            }
            Column {
                width: parent.width
                spacing: root.u
                visible: sync.provider === "folder"
                Field {
                    width: parent.width; u: root.u
                    label: "Ordner auf dem Gerät"
                    placeholder: "/home/root/sp-sync"
                    text: sync.localPath
                    onEdited: (t) => sync.localPath = t.trim()
                }
                Hint { text: "Ordner, den SPs „Lokale Datei“-Sync nutzt und der z. B. per Syncthing/rclone aufs Gerät gespiegelt wird." }
            }
            Field {
                width: parent.width; u: root.u
                visible: sync.provider !== ""
                label: "Verschlüsselungs-Passwort (falls in SP aktiviert)"
                password: true
                text: sync.encryptKey
                onEdited: (t) => sync.encryptKey = t
            }
            Hint { visible: !sync.encryptionSupported; text: "Diese Version wurde ohne Verschlüsselung gebaut." }

            Flow {
                width: parent.width
                spacing: root.u
                visible: sync.provider !== ""
                Text { text: "Automatisch:"; font.pixelSize: 2.6 * root.u; height: 5.5 * root.u; verticalAlignment: Text.AlignVCenter }
                Repeater {
                    model: [[0, "manuell"], [5, "5 min"], [10, "10 min"], [30, "30 min"]]
                    Chip { u: root.u; text: modelData[1]; checked: sync.intervalMinutes === modelData[0]; onClicked: sync.intervalMinutes = modelData[0] }
                }
            }
            Row {
                spacing: 2 * root.u
                visible: sync.provider !== ""
                EButton { u: root.u; text: sync.busy ? "Synchronisiere …" : "Jetzt synchronisieren"; enabled: sync.configured && !sync.busy; onClicked: sync.syncNow() }
            }
            Hint {
                visible: sync.provider !== ""
                text: (sync.status !== "" ? (sync.lastOk ? "" : "⚠ ") + sync.status + "\n" : "")
                      + "Letzter Sync: " + sync.lastSyncText + " · ausstehende Änderungen: " + app.pendingCount
            }

            Heading { text: "Updates" }
            Hint { text: "Installierte Version: " + updater.currentVersion + (updater.status !== "" ? "\n" + updater.status : "") }
            Row {
                spacing: 2 * root.u
                EButton { u: root.u; text: "Nach Updates suchen"; enabled: !updater.busy; onClicked: updater.check() }
                EButton { u: root.u; visible: updater.updateAvailable; checked: true; text: "Installieren"; enabled: !updater.busy; onClicked: updater.install() }
                EButton { u: root.u; visible: updater.readyToRestart; checked: true; text: "Neu starten"; onClicked: { app.stopTracking(); updater.restart() } }
            }

            Heading { text: "Daten" }
            Hint { text: "Datenordner: " + app.dataDir + "\nHandschrift bleibt auf dem Gerät (ink/), Aufgaben, Zeiten und Planung werden synchronisiert." }
        }
    }
}
