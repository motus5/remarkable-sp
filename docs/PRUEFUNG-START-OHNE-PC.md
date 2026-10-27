# Prüfung: App ohne PC starten (#15)

Stand 27.10.2026, reMarkable 1 mit OS 3.26.0.68, xovi und AppLoad installiert.

**Wunsch:** Die App in der Vorlesung nutzen, ohne PC. Heute muss xochitl per SSH gestoppt und
`start.sh` gestartet werden. Vom Gerät aus geht das nicht, weil literm in xochitl läuft und beim
Stopp mit beendet wird.

**Bedingung:** Nach dem Beenden kommt man zuverlässig zu xochitl zurück. Es entstehen keine neuen
Systemdienste, und nichts wird außerhalb der freigegebenen Ordner geschrieben.

## Was belegt ist

Quelle: README von [asivery/rm-appload](https://github.com/asivery/rm-appload) und
[`shim/`](https://github.com/asivery/rm-appload/tree/master/shim), gelesen am 27.10.2026.

- Apps liegen als eigener Ordner unter `/home/root/xovi/exthome/appload/<app>/`.
- **AppLoad kennt zwei Arten von Apps:**
  - **QML-App:** `manifest.json`, `icon.png`, `resources.rcc` und optional `backend/entry`. Die
    QML-Oberfläche läuft **im Prozess von xochitl**. Ein Backend ist ein eigener Prozess und spricht
    über einen Socket mit der Oberfläche.
  - **Externe App:** `external.manifest.json` mit den Feldern `name`, `application`,
    `workingDirectory`, `environment`, `args`, `qtfb`, `disablesWindowedMode` und `aspectRatio`.
    Mit `qtfb: true` legt AppLoad einen eigenen Bildspeicher an (Schlüssel in `QTFB_KEY`).
- **Shim:** Es gibt einen Shim für rM1/rM2-Programme (`QTFB_SHIM_MODEL` kennt RM1). Er stellt einen
  virtuellen Bildspeicher bereit und kann Stift, Touch und Tasten umleiten.
- **Beenden:** Eine Vollbild-App schließt man durch Ziehen von oben Mitte zur Mitte.
- **Gerätetest:** `systemctl stop xochitl` ohne eine startende App lässt das Tablet „eingefroren“
  aussehen; das ist im Test zweimal passiert ([LEARNINGS.md](LEARNINGS.md)).

## Optionen

| | Weg | Aufwand | Risiko | Offene Fragen |
|---|---|---|---|---|
| A | **QML-App in xochitl:** Oberfläche als AppLoad-QML; Zeichenfläche, Datenmodell und Sync in ein Backend | groß, weil die Zeichenfläche (C++) im Prozess von xochitl laufen müsste oder in QML neu entstehen müsste | Ein Fehler kann xochitl selbst abstürzen lassen | ob eigene C++-QML-Typen in xochitl überhaupt geladen werden können |
| B | **Externe App mit qtfb:** reMarkable SP läuft als eigener Prozess und zeichnet in den qtfb-Bildspeicher | mittel | gering, xochitl läuft weiter | ob es einen Weg für Qt 6 gibt (QPA-Plugin oder Shim) und ob er am rM1 läuft; wie Stift und Touch ankommen; ob die Latenz reicht |
| C | **Startknopf in AppLoad, der übergibt:** eine externe App ohne qtfb startet ein Skript. Das Skript beendet xochitl, startet `start.sh` und startet xochitl immer wieder, wenn die App endet (auch nach Absturz) | klein, die App bleibt unverändert | mittel: Hängt die App, steht das Tablet bis zum Neustart. Abhilfe: „App beenden“ (seit v0.3.3) plus Notausgang per Einschaltknopf | ob ein von xochitl gestartetes Skript den Stopp von xochitl überlebt (xochitl beendet seine Kindprozesse) – dafür bräuchte es `systemd-run` (vorübergehende Einheit, kein installierter Dienst). Ob das am Gerät vorhanden ist, ist unbelegt |

## Empfehlung

1. **Kurzfristig C** als Startknopf. Er ist klein und lässt die App unverändert, braucht aber vorher:
   - einen Gerätetest mit dem Besitzer am Gerät,
   - den Nachweis, dass `systemd-run` vorhanden ist,
   - einen Notausgang per Einschaltknopf in der App.
2. **Mittelfristig B** prüfen. Kann reMarkable SP sauber in qtfb laufen, muss xochitl nie mehr
   gestoppt werden. Das deckt der Recherche-Auftrag ab
   ([AUFTRAG-RECHERCHE-PLATTFORM.md](auftraege/AUFTRAG-RECHERCHE-PLATTFORM.md)).
3. **A nicht**, wegen Aufwand und Risiko für xochitl.

## Schon umgesetzt (v0.3.3)

- **„App beenden“** in den Einstellungen: Die App endet geordnet, Tinte ist gespeichert, die
  Zeiterfassung ist beendet. Das ist die Voraussetzung für jeden Weg.
- **Geordnetes Beenden** bei SIGINT, SIGTERM und SIGHUP (#14): Ein Startskript kann sich darauf
  verlassen, dass die App sauber endet.

## Nicht umgesetzt, Freigabe nötig

- **Freigabe des Besitzers:** Jeder Weg schreibt nach `/home/root/xovi/exthome/appload/`, und das
  braucht laut den Learnings seine ausdrückliche Freigabe.
- **Prüfbefehle, nur lesen, mit dem Besitzer am Gerät:**
  ```sh
  rM# command -v systemd-run; systemctl --version | head -n 1
  rM# ls /home/root/xovi/exthome/appload/
  rM# cat /home/root/xovi/exthome/appload/*/external.manifest.json 2>/dev/null | head -n 40
  ```
  Die letzte Zeile zeigt, wie die schon installierten externen Apps (z. B. KOReader) gestartet
  werden. Das ist das beste Vorbild für Weg C.
