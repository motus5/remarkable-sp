# Gerätetest reMarkable 1

Diese Datei ist die **vollständige Arbeitsgrundlage** für den Test am Gerät. Sie richtet sich an
eine Person oder einen Assistenten am Rechner des Besitzers, der per USB und SSH auf das
reMarkable 1 zugreift und das Projekt bisher nicht kennt.

**Vor dem ersten Befehl diese Datei ganz lesen.** Teil A erklärt Hintergrund und Regeln, Teil B
enthält die Testschritte.

---

# Teil A – Hintergrund und Regeln

## A1. Worum es geht

- **reMarkable SP** ist eine Aufgaben- und Zeiterfassungs-App für das reMarkable im Stil von
  [Super Productivity](https://github.com/johannesjo/super-productivity).
  - Geschrieben in C++/Qt 6 mit QML.
  - Bedienung mit Stift und Finger, Sync mit Super Productivity über WebDAV.
  - Repo: <https://github.com/motus5/remarkable-sp>, Branch `feature/remarkable-sp-app`,
    Release `v0.3.1`.
- **Bisher getestet:** am Linux-Desktop und unter ARM-Emulation (qemu) mit den Bibliotheken des
  offiziellen rM1-SDK.
- **Noch nie getestet: auf echter Hardware.** Genau das ist die Aufgabe hier.
- **Ziel des Tests:** herausfinden und berichten, ob und wie die App auf dem Gerät läuft.
  - **Nicht** Ziel: Fehler am Gerät beheben, Code ändern oder Systemdateien anpassen.
  - Korrekturen macht der Entwickler anhand des Berichts und liefert ein neues Release.

## A2. Das Gerät (Angaben des Besitzers)

| | |
|---|---|
| Modell | reMarkable 1 (SDK-Produktcode `rm1`), CPU armv7l |
| Firmware / Kernel | 3.26.0.68 / 5.4.70 (wird in B1 geprüft) |
| Shell-Werkzeuge | **BusyBox**: kürzere Optionen, z. B. `head -n 5` statt `head -5` |
| Verbindung | **nur USB**: `ssh root@10.11.99.1`, nicht über WLAN. Passwort unter *Einstellungen → Hilfe → Copyrights und Lizenzen* |
| Speicher | `/` zu **96 % voll (ca. 8 MB frei)**; `/home` hat mehrere GB frei |
| xovi | wird **nicht** über systemd geladen, sondern über das Skript `/home/root/xovi/start`, das xochitl selbst neu startet. Ausgelöst vom Timer `rm-delayed-hacks.timer` **10 Minuten nach dem Booten** |
| xovi-Erweiterungen | u. a. **appload**, **literm**, **touch-lock**, enable-typing-on-documents, navigate-using-arrow-keys |
| Eigener Sync des Besitzers | **rmfakecloud-proxy** auf `127.0.0.1:443` und ein Block `rmfake_start` in `/etc/hosts`. **Erwartet, kein Fehler, nicht anfassen** |

**Wichtige Begriffe:**
- **xochitl:** die normale reMarkable-Oberfläche, ein systemd-Dienst. Solange sie läuft, gehört
  ihr der Bildschirm. Für den Test wird sie kurz gestoppt.
- **xovi:** lädt Erweiterungen in xochitl, z. B. AppLoad. Nach `systemctl stop/start xochitl`
  läuft xochitl **ohne xovi**, AppLoad und die anderen Erweiterungen fehlen dann. Das ist im
  Test erwartet. Am Ende stellt `/home/root/xovi/start` den Normalzustand wieder her, aber
  **nur nach Freigabe des Besitzers** (B10).
- **Timer `rm-delayed-hacks.timer`:** startet xovi einmalig 10 Minuten nach dem Booten. Läuft das
  Gerät kürzer, würde der Timer mitten im Test xochitl neu starten. Darum die Uptime-Prüfung
  in B1.
- **`/home/root/.local/share/remarkable/xochitl`:** die **Notizbücher und Dokumente des
  Besitzers.** Einzige Ausnahme: das Backup in B0b und dessen Prüfung, beide nur lesend.

## A3. Was die App auf dem Gerät tut

- **Ort:** Sie liegt komplett in `/home/root/apps/remarkable-sp` (Hello-World:
  `/home/root/apps/hello-remarkable`). Daten schreibt sie nur in `…/remarkable-sp/data/`.
- **Systembibliotheken:** Sie nutzt die vorhandenen Qt-6-Bibliotheken und Plugins des Systems
  unter `/usr/lib` und verändert sie nicht.
- **Kein Autostart:** Sie startet nur von Hand über `start.sh` und läuft im Vordergrund der
  SSH-Sitzung. Strg+C beendet sie.
- **Bildschirm:** Sie zeichnet über reMarkables epaper-Plugin. Fehlt das System-Plugin
  `libepaper.so`, nimmt `start.sh` automatisch eine mitgelieferte Kopie aus dem App-Ordner,
  und zwar nur für diesen Prozess.
- **Touch:** Das epaper-Plugin übernimmt den Touchscreen **exklusiv**, solange die App läuft.
  Deshalb muss die App beendet sein, bevor xochitl wieder startet (Regel R6).
- **Stift:** Die App liest den Stift nur mit, ohne exklusiven Zugriff.
- **Netzwerk:** Ohne eingerichteten Sync greift sie nicht aufs Netz zu. Sync wird in diesem Test
  nicht eingerichtet.

## A4. Regeln (verbindlich)

| # | Regel |
|---|---|
| R1 | **Jeder Schritt am Gerät braucht die ausdrückliche Freigabe des Besitzers.** Vorher die geplanten Befehle vollständig zeigen, dann auf „ok“ warten. Ein „ok“ gilt nur für den gezeigten Schritt. |
| R2 | **Nur lesen, außer in den ausdrücklich genannten Schritten.** Geschrieben wird ausschließlich unter `/home/root/apps/`. |
| R3 | **Bei jeder Abweichung anhalten.** Ergebnis berichten, keine Lösung auf eigene Faust versuchen, keine Befehle außerhalb dieser Anleitung „zum Ausprobieren“. Ausnahme: die in Schritt B7 vorgesehenen Varianten. |
| R4 | Bildschirm-Beobachtungen kann nur der Besitzer machen. Danach fragen und seine Antwort wörtlich in den Bericht übernehmen. |
| R5 | Nichts behaupten, was nicht geprüft ist. Leere Ausgabe gilt als Fehlschlag, nicht als Erfolg. |
| R6 | **Vor jedem `systemctl start xochitl` prüfen, dass keine App mehr läuft:** `pidof remarkable-sp hello_remarkable` muss leer sein. |
| R7 | Am Ende muss das Gerät so dastehen wie vorher: xochitl läuft, mit xovi, wenn es vorher mit xovi lief. Der freie Speicher entspricht dem Anfangswert, oder die App bleibt auf Wunsch des Besitzers installiert. |
| R8 | Die Datei-Übertragung läuft nur über `deploy/install.sh` und `deploy/uninstall.sh`. Diese Skripte vorher lesen, sie sind kurz. |
| R9 | Befehle auf dem Gerät in BusyBox-Schreibweise (z. B. `head -n 5`). |
| R10 | Vor dem ersten Schreibschritt ist das Backup (B0b) fertig und geprüft. |

## A5. Verbotene Aktionen

Keine dieser Aktionen ausführen, auch nicht „zur Diagnose“. Wenn eine davon nötig scheint:
anhalten und berichten.

- Schreiben, Löschen oder Ändern in `/usr`, `/lib`, `/etc`, `/opt`, `/var`, `/boot` und allem
  außerhalb von `/home/root/apps`, also auch kein `mount -o remount,rw`.
- Zugriff auf `/home/root/.local/share/remarkable/` (Dokumente des Besitzers). Ausnahme: das
  Backup in B0b, nur lesend.
- Änderungen an `/home/root/xovi`. `/home/root/xovi/start` nur in B10 und nur nach Freigabe.
- rmfakecloud-proxy, den `rmfake_start`-Block in `/etc/hosts` und die prx-Connectoren: nichts
  daran ändern, nichts neu starten, nicht als Fehler werten.
- WLAN-Einstellungen; gearbeitet wird nur über USB.
- `systemctl enable|disable|mask|edit|daemon-reload`, Unit-Dateien anlegen oder ändern, Cronjobs.
- `reboot`, `poweroff`, `shutdown`, `rm -rf` mit Pfaden außerhalb von `/home/root/apps/…`.
- Paketmanager (`opkg`, `toltec`, `vellum`), Updates, Downloads auf dem Gerät.
- Passwörter oder SSH-Konfiguration ändern. Ausnahme: ein SSH-Schlüssel, **nur** wenn der
  Besitzer ihn ausdrücklich wünscht.
- `dd`, `fsck`, `mkfs` und alles, was Partitionen oder `/dev/mmc*` berührt.
- Schreiben auf Eingabe- oder Grafikgeräte (`/dev/input/*`, `/dev/fb0`). Lesen übernimmt die App
  selbst.

## A6. Notfallplan

| Situation | Maßnahme |
|---|---|
| App reagiert nicht, Strg+C wirkt nicht | zweite SSH-Sitzung: `pidof remarkable-sp hello_remarkable`, dann `kill <pid>`, wenn nötig `kill -9 <pid>` |
| SSH-Sitzung abgebrochen, während die App lief | neu verbinden, `pidof …`, App beenden, dann `systemctl start xochitl` |
| xochitl startet nicht (`systemctl is-active xochitl` ≠ `active`) | einmal `systemctl start xochitl`; danach `systemctl status xochitl --no-pager` und `journalctl -u xochitl -n 50 --no-pager` lesen und **anhalten**, berichten |
| xochitl läuft, aber AppLoad fehlt | erwartet nach `systemctl start xochitl` (xovi nicht geladen). Wiederherstellen erst in B10 mit `/home/root/xovi/start`, nur nach Freigabe |
| Touch reagiert nicht (B4/B7) | App beenden. Zuerst ausschließen, dass die xovi-Erweiterung **touch-lock** aktiv ist: Besitzer prüft in den Quick-Settings von xochitl, dafür vorher xochitl (und nach Freigabe xovi) starten. Dann den Schritt wiederholen |
| Bildschirm bleibt hängen, nichts geht mehr | Besitzer: Ein-/Aus-Taste 10 s halten (Neustart). Die App startet danach nicht von selbst |
| Gerät per USB nicht mehr erreichbar | Kabel neu stecken, Gerät entsperren. Bleibt es so: anhalten, berichten |

**xovi-Zustand bestimmen (nur lesen, Teil von B1, B5, B8, B10):**
```sh
rM# P=$(pidof xochitl); [ -n "$P" ] && tr '\0' '\n' < /proc/$P/environ | grep -i preload || echo "kein LD_PRELOAD"
```
- Ein `LD_PRELOAD` mit xovi-Pfad bedeutet: xochitl läuft mit xovi.
- Vor dem Test ist das der Normalzustand. Nach `systemctl start xochitl` ist `kein LD_PRELOAD`
  erwartet.

## A7. Vorbereitung am PC

- **Linux:** `git`, `ssh`, `scp`, `tar`, `gzip`, `sha256sum`.
- **Windows:** dieselben Werkzeuge in **Git Bash** oder WSL, weil die Skripte Shell-Skripte sind.
- **Assistent als Tester:** im normalen Rechte-Modus laufen, sodass vor jedem Befehl gefragt wird.
  Keine automatische Freigabe für Shell-Befehle.
- **SSH-Passwort:** Ohne Schlüssel fragt jeder `ssh`/`scp`-Aufruf nach dem Passwort. Das ist in
  Ordnung. Einen Schlüssel nur auf ausdrücklichen Wunsch des Besitzers einrichten.

---

# Teil B – Testschritte

Pro Schritt gilt dasselbe:
1. Befehle zeigen.
2. Freigabe abwarten.
3. Ausführen.
4. Ausgabe vollständig zeigen.
5. Mit der Erwartung vergleichen und ja/nein angeben.

`PC$` bedeutet: am Rechner im Repo-Ordner. `rM#` bedeutet: auf dem Gerät (`ssh root@10.11.99.1`).

**Gebaut mit:** dem offiziellen SDK
`remarkable-production-image-5.6.75-rm1-public-x86_64-toolchain.sh` für reMarkable OS
3.26.0.68 (Qt 6.8.2), Quelle <https://developer.remarkable.com/links>.

## B0. Dateien holen und prüfen (PC, ohne Gerät)

```sh
PC$ git clone -b feature/remarkable-sp-app https://github.com/motus5/remarkable-sp && cd remarkable-sp
PC$ for f in SHA256SUMS hello-remarkable-rm1.tar.gz remarkable-sp-rm1-0.3.1.tar.gz; do
      curl -fsSLO https://github.com/motus5/remarkable-sp/releases/download/v0.3.1/$f; done
PC$ sha256sum -c --ignore-missing SHA256SUMS
PC$ cat deploy/install.sh deploy/uninstall.sh
```
**Erwartet:**
- `hello-remarkable-rm1.tar.gz: OK` und `remarkable-sp-rm1-0.3.1.tar.gz: OK`.
- Die Skripte schreiben nur nach `/home/root/apps`.

## B0b. Backup der Dokumente (ASUS, Pflicht vor dem ersten Schreibschritt)

- Das vorhandene Backup-Skript des Besitzers auf dem **ASUS** ausführen, nicht über Mac/SMB:
  `~/Dokumente/bue-iot-rm1-bkp/rm1-backup.sh`.
- Vorher dem Besitzer zeigen, welche Zeile den **RUN-Pfad** festlegt, und sie mit Freigabe auf
  das heutige Datum setzen.
- In `screen` starten, damit ein Verbindungsabbruch das Backup nicht abbricht:
  ```sh
  PC$ screen -S rm1-backup ~/Dokumente/bue-iot-rm1-bkp/rm1-backup.sh
  ```
- **Prüfung:** Die Zahl der `.metadata`-Dateien auf dem Gerät muss gleich der im Backup sein.
  ```sh
  rM# find /home/root/.local/share/remarkable/xochitl -name '*.metadata' | wc -l
  PC$ find <RUN-Pfad> -name '*.metadata' | wc -l
  ```
  **Erwartet:** beide Zahlen gleich. Sonst **anhalten**.

## B1. Vorab-Prüfungen (nur lesen)

```sh
rM# uptime
rM# systemctl list-timers --all --no-pager | grep -i delayed
rM# cat /etc/os-release; uname -a; df -h / /home
rM# grep -i version /usr/share/remarkable/update.conf 2>/dev/null
rM# cat /sys/class/power_supply/*/capacity 2>/dev/null
rM# ls -l /usr/lib/libQt6Core.so.6 /usr/lib/libQt6Gui.so.6 /usr/lib/libQt6Qml.so.6 \
          /usr/lib/libQt6Quick.so.6 /usr/lib/libQt6Network.so.6 /usr/lib/libcrypto.so.3 /usr/lib/libz.so.1
rM# ls /usr/lib/plugins/platforms/ /usr/lib/plugins/scenegraph/
rM# grep -E '^N: Name|^H: Handlers' /proc/bus/input/devices
rM# systemctl is-active xochitl
rM# ls -la /home/root/apps 2>/dev/null || echo "kein apps-Ordner"
```
Dazu die **xovi-Prüfung aus A6** ausführen. **Zuerst die Uptime prüfen:** Läuft das Gerät
weniger als 10 Minuten, warten, bis der Timer `rm-delayed-hacks` gelaufen ist (xovi aktiv),
dann B1 neu beginnen.

| Prüfung | Erwartet | Wenn nicht |
|---|---|---|
| `uptime` | mindestens 10 Minuten | warten, dann neu |
| `os-release` | `IMG_VERSION="5.6.75"` (SDK-Stand zu 3.26.0.68); andere Werte notieren | weiter, Version berichten |
| `uname -a` | Kernel `5.4.70`, `armv7l` | anderer Kernel: notieren; kein armv7l: **anhalten** |
| `df -h / /home` | `/` ca. 96 % (Wert notieren, darf sich im Test nicht ändern); `/home` mindestens **10 MB** frei | `/home` zu voll: **anhalten** |
| Akku | mindestens 30 % | Besitzer laden lassen |
| Qt-6-Bibliotheken | alle 7 Dateien vorhanden | **anhalten**, fehlende nennen |
| `plugins/scenegraph` | enthält `libqsgepaper.so` | **anhalten** |
| `plugins/platforms` | enthält `libepaper.so`; fehlt sie, nutzen die Apps automatisch die mitgelieferte | weiter, notieren |
| Eingabegeräte | Wacom-Digitizer (Stift), Touchscreen, Tasten; Namen und `eventN` notieren | weiter, Liste berichten |
| xochitl | `active` | **anhalten** |
| xovi | `LD_PRELOAD` mit xovi-Pfad (Normalzustand) | notieren |

## B2. Hello-World installieren (PC)

```sh
PC$ deploy/install.sh hello-remarkable-rm1.tar.gz
```
**Erwartet:**
- `Installiert: 0.3.1 in /home/root/apps/hello-remarkable`.
- Die Liste zeigt `hello_remarkable`, `start.sh`, `fallback/`, `licenses/`, `VERSION`.
- `rM# df -h /home` zeigt etwa 0,3 MB weniger frei.

## B3. xochitl stoppen (eigener Schritt)

```sh
rM# systemctl stop xochitl; systemctl is-active xochitl
```
**Erwartet:** `inactive`. Der Bildschirm zeigt weiter das letzte Bild, das ist normal.

## B4. Hello-World starten und beenden

```sh
rM# /home/root/apps/hello-remarkable/start.sh
```
**Erwartet (Besitzer fragen):**
- Der Bildschirm wird weiß, mit dickem schwarzem Rahmen und dem Text **„Hello reMarkable 1“**.
- Darunter eine Zeile mit der Bildschirmgröße (1404 × 1872) und ein schwarzes Quadrat.
- **Jedes Tippen mit dem Finger** blendet das Quadrat aus bzw. ein.
- `start: using bundled epaper platform plugin` im Terminal bedeutet: Die mitgelieferte Kopie
  wird benutzt (vgl. B1).

**Beenden** mit **Strg+C**, danach `rM# pidof hello_remarkable` → muss leer sein.

**Berichten:**
- Bild ja/nein, Text lesbar ja/nein, Tippen wirkt ja/nein.
- Die komplette Terminal-Ausgabe.
- Kein Bild: anhalten, Ausgabe berichten, weiter mit B5 (xochitl zurückholen).

## B5. xochitl starten (eigener Schritt)

```sh
rM# pidof remarkable-sp hello_remarkable || echo "keine App aktiv"
rM# systemctl start xochitl; sleep 5; systemctl is-active xochitl
```
**Erwartet:**
- `keine App aktiv`, danach `active`, und die normale Oberfläche erscheint.
- Dann die **xovi-Prüfung aus A6**: `kein LD_PRELOAD` ist hier **erwartet**. AppLoad fehlt bis B10,
  das ist kein Fehler.

## B6. App installieren (PC)

```sh
PC$ deploy/install.sh remarkable-sp-rm1-0.3.1.tar.gz
```
**Erwartet:** `Installiert: 0.3.1 in /home/root/apps/remarkable-sp`, rund 2,7 MB belegt.

## B7. xochitl stoppen, App starten und prüfen

```sh
rM# systemctl stop xochitl; systemctl is-active xochitl
rM# /home/root/apps/remarkable-sp/start.sh
```
**Erwartet im Terminal:**
`Pen digitizer /dev/input/eventN (x 0..…, y 0..…, pressure 0..…), transform swapxy,invy`

**Erwartet auf dem Bildschirm:** links eine Icon-Leiste, oben „Heute“, rechts ein runder ＋-Knopf.

Mit dem Besitzer prüfen und je Punkt ja/nein berichten:

| # | Aktion | Erwartet |
|---|---|---|
| a | ＋ mit dem Finger antippen | Notizbuchseite öffnet sich, links die Werkzeugleiste |
| b | Mit dem Stift oben links ins Titelfeld schreiben | Tinte erscheint **direkt unter der Stiftspitze** |
| c | Mit dem Radierer-Ende über die Tinte | Tinte verschwindet |
| d | Handballen auf das Papier legen | keine Striche |
| e | ⌨ antippen | Bildschirmtastatur erscheint, Tippen schreibt ins Feld |
| f | „Schließen“ unten links | zurück zur Liste, Aufgabe mit Handschrift-Titel sichtbar |
| g | Aufgabe lange drücken | Kontextmenü erscheint |
| h | Wie schnell erscheint die Tinte beim Schreiben? | Eindruck des Besitzers notieren |

**Erlaubte Varianten, wenn (b) nicht stimmt:**
1. App mit Strg+C beenden.
2. Besitzer fragen, wo die Tinte statt unter der Spitze erscheint (gespiegelt, gedreht).
3. Nacheinander mit Freigabe starten, bis die Tinte unter der Spitze erscheint:
   ```sh
   rM# RMSP_PEN_TRANSFORM="swapxy,invx" /home/root/apps/remarkable-sp/start.sh
   ```
   Werte in dieser Reihenfolge: `swapxy,invx` · `swapxy` · `swapxy,invx,invy` · `invy` · `invx` ·
   `invx,invy` · `none`
4. Den passenden Wert berichten.

**Touch reagiert gar nicht:** zuerst touch-lock ausschließen (siehe A6), erst dann die Varianten
unten.

**Erlaubte Varianten, wenn (a) nicht stimmt** (Tipp landet woanders): vor den Start
`QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="rotate=0"` setzen, dann `rotate=90`, dann `rotate=270`, und
berichten, welcher Wert stimmt.

**Beenden** mit **Strg+C**, dann:
```sh
rM# pidof remarkable-sp || echo "beendet"
rM# ls -la /home/root/apps/remarkable-sp/data
```
**Erwartet:** `beendet`; in `data/` liegen `sp-state.json`, `sp-client.json` und `ink/`.

## B8. xochitl starten (eigener Schritt)

Wie B5, einschließlich der xovi-Prüfung.

## B9. Rückbau (PC), nur auf Wunsch des Besitzers

Den Besitzer fragen: App behalten (für weitere Tests oder später für AppLoad) oder entfernen?

```sh
PC$ deploy/uninstall.sh hello-remarkable
PC$ deploy/uninstall.sh remarkable-sp                          # Programm weg, data/ bleibt
PC$ deploy/uninstall.sh remarkable-sp root@10.11.99.1 --purge  # alles weg
rM# ls -la /home/root/apps 2>/dev/null || echo "apps-Ordner entfernt"; df -h / /home
```
**Erwartet:**
- „Entfernt: …“ bzw. „Programm entfernt, Daten behalten“.
- `df` für `/` ist unverändert gegenüber B1. `/home` entspricht B1, wenn alles entfernt wurde.

## B10. Abschluss und xovi wiederherstellen

Erst prüfen:
```sh
rM# systemctl is-active xochitl; pidof remarkable-sp hello_remarkable || echo "keine App aktiv"
```
**Erwartet:** `active`, `keine App aktiv`.

Dann **nur nach Freigabe des Besitzers** xovi wieder laden. Das Skript startet xochitl selbst neu:
```sh
rM# /home/root/xovi/start
```
**Erwartet:**
- Nach kurzer Zeit erscheint wieder die normale Oberfläche mit AppLoad (Besitzer fragen).
- Die xovi-Prüfung aus A6 zeigt wieder `LD_PRELOAD` wie in B1.
- `df -h /` entspricht dem Wert aus B1.

---

## Bericht

Als Datei `BERICHT-RM1.md` im Repo-Ordner am PC, nicht auf dem Gerät. Danach durch Rücklesen
prüfen. Inhalt:

1. **Kurzfazit:** Hello-World läuft ja/nein, App läuft ja/nein, Stift richtig ja/nein, Touch
   richtig ja/nein.
2. **Pro Schritt B0–B10:** Befehle, vollständige Ausgabe, Bildschirm-Beobachtung des Besitzers,
   „entspricht Erwartung: ja/nein“.
3. **Für den Entwickler besonders wichtig:**
   - die Ausgaben aus B1 (Versionen, Plugins, Eingabegeräte, xovi-Zustand),
   - die Terminal-Ausgaben aus B4 und B7,
   - falls nötig der passende `RMSP_PEN_TRANSFORM`- und Touch-Wert,
   - der Eindruck zur Geschwindigkeit (B7 h).
4. **Endzustand:** Backup geprüft (Zahl der `.metadata`), xochitl aktiv, xovi wieder geladen,
   App installiert oder entfernt, freier Speicher auf `/` und `/home`.
