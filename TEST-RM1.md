# Gerätetest reMarkable 1

Diese Datei ist die **vollständige Arbeitsgrundlage** für den Test am Gerät. Sie richtet sich an
eine Person oder einen Assistenten am Rechner des Besitzers, der per SSH (USB oder WLAN, siehe A8)
auf das reMarkable 1 zugreift und das Projekt bisher nicht kennt.

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
    Release **`v0.3.2`**.
- **Erster Gerätetest (v0.3.1, 08.–10.10.2026):** App läuft, Stift und Touch treffen. Befunde:
  Oberfläche nur ein Viertel groß, Stift ca. 400 ms verzögert, Titelfeld, Dateibesitz. Bericht:
  `docs/geraetetest/2026-10-rm1-v0.3.1/`.
- **Neu in v0.3.2:** Vollbild (#3), Stift-Latenz messbar und Tinte ohne Kantenglättung (#4),
  Titelfeld (#5), Archive gehören root (#6).
- **Ziel dieses Tests:** die Korrekturen am Gerät bestätigen und die Stift-Latenz **messen**
  (B7 h), dazu die im ersten Test ausgelassenen Punkte B7 f und g.
  - **Nicht** Ziel: Fehler am Gerät beheben, Code ändern oder Systemdateien anpassen.
  - Korrekturen macht der Entwickler anhand des Berichts und liefert ein neues Release.

## A2. Das Gerät (Angaben des Besitzers)

| | |
|---|---|
| Modell | reMarkable 1 (SDK-Produktcode `rm1`), CPU armv7l |
| Firmware / Kernel | 3.26.0.68 / 5.4.70 (wird in B1 geprüft) |
| Shell-Werkzeuge | **BusyBox**: kürzere Optionen, z. B. `head -n 5` statt `head -5` |
| Verbindung | USB `ssh root@10.11.99.1`, oder **WLAN** (Adresse per DHCP, am Gerät unter *Einstellungen → Hilfe* nachsehen, nicht raten), wenn USB keine Daten überträgt – im ersten Test der Fall (Kabel/Buchse). Für WLAN gilt A8. Passwort unter *Einstellungen → Hilfe → Copyrights und Lizenzen*, **niemals in Dateien des Repos oder in den Bericht schreiben** |
| Speicher | `/` zu **97–98 % voll (ca. 4–6 MB frei)** und schrumpft von selbst, vor allem durch `/var/log/journal` (im ersten Test 7,5 MB); `/home` hat mehrere GB frei |
| xovi | Start über `/home/root/xovi/start`, ausgelöst vom Timer `rm-delayed-hacks.timer` **10 Minuten nach dem Booten**. Das Skript hängt einen tmpfs-Mount auf `/etc/systemd/system/xochitl.service.d/` mit `00-xovi.conf` (`LD_PRELOAD=/home/root/xovi/xovi.so`) ein. **Dieser Mount hält bis zum nächsten Neustart des Geräts** |
| xovi-Erweiterungen | u. a. **appload**, **literm**, **touch-lock**, enable-typing-on-documents, navigate-using-arrow-keys |
| Eigener Sync des Besitzers | **rmfakecloud-proxy** auf `127.0.0.1:443` und ein Block `rmfake_start` in `/etc/hosts`. **Erwartet, kein Fehler, nicht anfassen** |

**Wichtige Begriffe:**
- **xochitl:** die normale reMarkable-Oberfläche, ein systemd-Dienst. Solange sie läuft, gehört
  ihr der Bildschirm. Für den Test wird sie kurz gestoppt.
- **xovi:** lädt Erweiterungen in xochitl, z. B. AppLoad und touch-lock.
  - Wegen des tmpfs-Mounts (siehe Tabelle) bleibt xovi auch nach `systemctl stop/start xochitl`
    aktiv. Nach jedem xochitl-Start sind AppLoad und touch-lock wieder da. Das ist **erwartet**.
  - `/home/root/xovi/start` ist nur nötig, wenn `LD_PRELOAD` fehlt (B10), und dann nur nach
    Freigabe des Besitzers.
- **Timer `rm-delayed-hacks.timer`:** startet xovi einmalig 10 Minuten nach dem Booten. Läuft das
  Gerät kürzer, würde der Timer mitten im Test xochitl neu starten. Darum die Uptime-Prüfung
  in B1.
- **`/home/root/.local/share/remarkable/xochitl`:** die **Notizbücher und Dokumente des
  Besitzers.** Einzige Ausnahme: ein Backup mit `rm1-backup-v2.sh` und dessen Prüfung, beide nur
  lesend, und nur wenn der Besitzer ausdrücklich ein neues Backup will (siehe B0b).

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
- **Stift:** Die App liest den Stift nur mit, ohne exklusiven Zugriff. Tinte ist auf dem Gerät rein
  schwarz (ohne Kantenglättung). Mit `RMSP_LATENCY_LOG=1` schreibt sie nach jedem Strich eine
  Messzeile `LATENZ …` ins Terminal (B7 h).
- **Bekannte, harmlose Terminal-Meldungen** (kamen im ersten Test, Bild trotzdem korrekt):
  `Could not open bin file for reading`, `rm.epaperkeyboardhandler: Failed to open pogo lang
  status` / `No keymap … defaulting to US`, `QFSFileEngine::map: Mapping a file beyond its size
  is not portable`. Nur im Bericht nennen, nicht als Fehler werten.
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
| R7 | Am Ende muss das Gerät so dastehen wie vorher: xochitl läuft mit xovi (`LD_PRELOAD` vorhanden). Außerhalb von `/home/root/apps` sind keine Dateien durch den Test entstanden (B8b, Systemeinträge ausgenommen). Der freie Platz auf `/` sinkt höchstens um das **Wachstum von `/var/log/journal` plus 1 MB** (beides in B1 und B8b messen). Die App bleibt nur auf Wunsch des Besitzers installiert. |
| R12 | **xochitl nur stoppen, wenn der Besitzer am Gerät ist.** Ab `systemctl stop xochitl` ist das Tablet unbedienbar, bis B5 bzw. B8. B3→B4→B5 und B3/B7→B8 laufen **am Stück**, nie über eine Pause. Vor jeder Pause: xochitl aktiv, keine App. |
| R8 | Die Datei-Übertragung läuft nur über `deploy/install.sh` und `deploy/uninstall.sh`. Diese Skripte vorher lesen, sie sind kurz. |
| R9 | Befehle auf dem Gerät in BusyBox-Schreibweise (z. B. `head -n 5`). |
| R10 | Das Backup ist vorhanden und geprüft (B0b). **Nicht erneut ausführen**, nur auf ausdrücklichen Wunsch. |
| R11 | **Kein Passwort** in Repo-Dateien, `BERICHT-RM1.md`, Commits oder Ausgaben, die in den Bericht kopiert werden. Das Repo ist öffentlich. |

## A5. Verbotene Aktionen

Keine dieser Aktionen ausführen, auch nicht „zur Diagnose“. Wenn eine davon nötig scheint:
anhalten und berichten.

- Schreiben, Löschen oder Ändern in `/usr`, `/lib`, `/etc`, `/opt`, `/var`, `/boot` und allem
  außerhalb von `/home/root/apps`, also auch kein `mount -o remount,rw`.
- Zugriff auf `/home/root/.local/share/remarkable/` (Dokumente des Besitzers). Ausnahme: ein
  vom Besitzer gewünschtes Backup (B0b), nur lesend.
- Änderungen an `/home/root/xovi` und am tmpfs-Mount `/etc/systemd/system/xochitl.service.d/`
  (lesen erlaubt). `/home/root/xovi/start` nur in B10, nur wenn `LD_PRELOAD` fehlt, und nur nach
  Freigabe.
- rmfakecloud-proxy, den `rmfake_start`-Block in `/etc/hosts` und die prx-Connectoren: nichts
  daran ändern, nichts neu starten, nicht als Fehler werten.
- WLAN-Einstellungen und Ruhezustand am Gerät ändern (WLAN nur nutzen, siehe A8).
- `systemctl enable|disable|mask|edit|daemon-reload`, Unit-Dateien anlegen oder ändern, Cronjobs.
- `reboot`, `poweroff`, `shutdown`, `rm -rf` mit Pfaden außerhalb von `/home/root/apps/…`.
- Paketmanager (`opkg`, `toltec`, `vellum`), Updates, Downloads auf dem Gerät.
- Passwörter oder SSH-Konfiguration ändern. Ausnahme: ein SSH-Schlüssel, **nur** wenn der
  Besitzer ihn ausdrücklich wünscht.
- `dd`, `fsck`, `mkfs` und alles, was Partitionen oder `/dev/mmc*` berührt.
- Schreiben auf Eingabe- oder Grafikgeräte (`/dev/input/*`, `/dev/fb0`). Erlaubt ist nur das
  **Lesen** von `/dev/fb0` in B7 (Bildschirmabzug).

## A6. Notfallplan

| Situation | Maßnahme |
|---|---|
| App reagiert nicht, Strg+C wirkt nicht | zweite SSH-Sitzung: `pidof remarkable-sp hello_remarkable`, dann `kill <pid>`, wenn nötig `kill -9 <pid>` |
| SSH-Sitzung abgebrochen, während die App lief | neu verbinden, `pidof …`, App beenden, dann `systemctl start xochitl` |
| xochitl startet nicht (`systemctl is-active xochitl` ≠ `active`) | einmal `systemctl start xochitl`; danach `systemctl status xochitl --no-pager` und `journalctl -u xochitl -n 50 --no-pager` lesen und **anhalten**, berichten |
| xochitl läuft, aber AppLoad fehlt | xovi-Prüfung (unten). Fehlt `LD_PRELOAD`: **anhalten**, berichten. Wiederherstellen in B10 mit `/home/root/xovi/start`, nur nach Freigabe |
| Touch reagiert nicht (B4/B7) | App beenden. Zuerst ausschließen, dass die xovi-Erweiterung **touch-lock** aktiv ist: Besitzer prüft in den Quick-Settings von xochitl, dafür xochitl starten (B5, xovi kommt automatisch mit). Dann den Schritt wiederholen |
| Bildschirm bleibt hängen, nichts geht mehr | Besitzer: Ein-/Aus-Taste 10 s halten (Neustart). Die App startet danach nicht von selbst |
| Gerät per USB nicht mehr erreichbar | Kabel neu stecken, Gerät entsperren. Bleibt es so: anhalten, berichten |
| Gerät per WLAN nicht erreichbar | 3–5 Pings; Tablet entsperren, Ladekabel prüfen; IP am Gerät nachsehen. Bleibt es so: anhalten, berichten |
| Verbindung bricht ab, während die App läuft | Läuft sie mit `ssh -t` (A8), beendet das Hangup-Signal sie. Neu verbinden, `pidof` prüfen, dann B5/B8 |

**xovi-Zustand bestimmen (nur lesen, Teil von B1, B5, B8, B10):**
```sh
rM# P=$(pidof xochitl); [ -n "$P" ] && tr '\0' '\n' < /proc/$P/environ | grep -i preload || echo "kein LD_PRELOAD"
```
- **Erwartet immer**, also vor dem Test und nach jedem xochitl-Start:
  `LD_PRELOAD=/home/root/xovi/xovi.so`.
- Die Ausgabe `kein LD_PRELOAD` ist eine Abweichung.
- Ergänzend, nur lesen: `ls /etc/systemd/system/xochitl.service.d/` zeigt `00-xovi.conf`.

## A7. Vorbereitung am PC

- **Linux:** `git`, `ssh`, `scp`, `tar`, `gzip`, `sha256sum`.
- **Windows:** dieselben Werkzeuge in **Git Bash** oder WSL, weil die Skripte Shell-Skripte sind.
- **Eine Maschine:** Der Test läuft auf genau dem Rechner, an dem das **USB-Kabel** des rM1 steckt.
- **Lokaler Klon:** Der Klon muss auf einer **lokalen Platte** liegen, nicht auf einem
  Netzlaufwerk (SMB, z. B. `/Volumes/…`).
- **Aktueller Stand:** Vor dem Start `git pull` auf `feature/remarkable-sp-app` ausführen und
  prüfen, dass `grep 'VERSION 0.3.2' CMakeLists.txt` einen Treffer liefert und `git log --oneline`
  den Commit „TEST-RM1.md: Korrekturen aus dem ersten Gerätetest (#7)“ enthält. Sonst ist die
  Anleitung veraltet: **anhalten**.
- **Assistent als Tester:** im normalen Rechte-Modus laufen, sodass vor jedem Befehl gefragt wird.
  Keine automatische Freigabe für Shell-Befehle.
- **SSH-Passwort:** Ohne Schlüssel fragt jeder `ssh`/`scp`-Aufruf nach dem Passwort. Das ist in
  Ordnung. Einen Schlüssel nur auf ausdrücklichen Wunsch des Besitzers einrichten.
- **Python 3** am PC für den Bildschirmabzug (`deploy/fb0-to-png.py`, ohne Zusatzpakete).

## A8. Test über WLAN oder einen nicht-interaktiven Kanal

Im ersten Test gingen über USB keine Daten (Kabel/Buchse), und die Test-Instanz hatte kein
interaktives Terminal. Bewährt hat sich:

- **Eine `screen`-Sitzung auf dem PC** (z. B. `rm1test`), in der alle Gerätebefehle laufen; je
  Schritt ein Skript, Ausgabe in eine Protokolldatei **mit Zeitstempel im Namen** (nie
  überschreiben). Passwort nur als Umgebungsvariable dieser Sitzung, Shell-Historie dort aus.
- **Apps mit `ssh -t` starten** (mit Terminal): Nur dann kommt Strg+C an, und ein
  Verbindungsabbruch beendet die App über das Hangup-Signal.
- **Strg+C** ohne interaktives Terminal: `screen -S rm1test -X stuff $'\003'`.
  Ersatzweg: `ssh <tablet> 'kill $(pidof remarkable-sp hello_remarkable)'`, danach `pidof` prüfen.
- **WLAN wach halten:** Tablet am Ladekabel; vor jedem Geräteschritt 3–5 Pings; während des Tests
  eine Ping-Schleife in einer eigenen screen-Sitzung (nur lesend):
  `screen -dmS rm1keepalive sh -c 'while true; do ping -c1 -W1 <tablet-ip> >/dev/null; sleep 5; done'`,
  nach dem Test `screen -S rm1keepalive -X quit`. SSH mit `-o ServerAliveInterval=10`.
- **Jeder Geräteschritt** beginnt mit einem Ping und endet mit `systemctl is-active xochitl`, so
  fällt ein offener Teststand (xochitl gestoppt) sofort auf.
- **Nach einem Neustart des PCs** sind die screen-Sitzungen weg: zuerst den Gerätezustand lesen
  (xochitl, `pidof`), dann weiter.
- `deploy/install.sh`/`uninstall.sh` bekommen die Adresse als zweites Argument
  (`root@<tablet-ip>`).

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

Den Klon gibt es schon (A7), falls nicht: `git clone -b feature/remarkable-sp-app
https://github.com/motus5/remarkable-sp`.

```sh
PC$ for f in SHA256SUMS hello-remarkable-rm1.tar.gz remarkable-sp-rm1-0.3.2.tar.gz soll-bild-rm1-0.3.2.png; do
      curl -fsSLO https://github.com/motus5/remarkable-sp/releases/download/v0.3.2/$f; done
PC$ sha256sum -c --ignore-missing SHA256SUMS
PC$ device/check-archive.sh hello-remarkable-rm1.tar.gz hello-remarkable 1024
PC$ device/check-archive.sh remarkable-sp-rm1-0.3.2.tar.gz remarkable-sp 4096
PC$ cat deploy/install.sh deploy/uninstall.sh
```
**Erwartet:**
- `hello-remarkable-rm1.tar.gz: OK` und `remarkable-sp-rm1-0.3.2.tar.gz: OK`.
- Zweimal `T6 ok: … Besitzer 0/0`.
- Die Skripte schreiben nur nach `/home/root/apps`.
- `soll-bild-rm1-0.3.2.png` zeigt, wie die App auf dem Gerät aussehen soll (Emulation, volle
  Fläche 1404 × 1872).

## B0b. Backup der Dokumente: erledigt

- **Vorhanden und geprüft:** `~/Dokumente/bue-iot-rm1-bkp/rm1-2026-10-08-0131/` auf dem ASUS,
  42 von 42 Dokumenten, geprüft über die Zahl der `.metadata`-Dateien.
- **Nicht erneut ausführen.** Im Bericht nur diesen Ordner nennen.
- Will der Besitzer ausdrücklich ein neues Backup: auf dem ASUS, in `screen`, mit
  `~/Dokumente/bue-iot-rm1-bkp/rm1-backup-v2.sh 192.168.89.177`, wobei das Argument die IP ist.
  Danach die Zahl der `.metadata`-Dateien auf Gerät und Backup vergleichen.

## B1. Vorab-Prüfungen (nur lesen)

```sh
rM# uptime
rM# systemctl list-timers --all --no-pager | grep -i delayed
rM# cat /etc/os-release; uname -a; df -k / /home
rM# du -sk /var/log/journal
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
| `os-release` | `IMG_VERSION="3.26.0.68"`, `VERSION_ID=5.6.75` (OS-Image zu 3.26.0.68); andere Werte notieren | weiter, Version berichten |
| `uname -a` | Kernel `5.4.70`, `armv7l` | anderer Kernel: notieren; kein armv7l: **anhalten** |
| `df -k / /home` | `/` ca. 97–98 %, freien Wert in KB **notieren** (Referenz für R7); `/home` mindestens **10 MB** frei | `/home` zu voll: **anhalten** |
| `du -sk /var/log/journal` | Wert notieren (Referenz für R7) | – |
| Akku | mindestens 30 % | Besitzer laden lassen |
| Qt-6-Bibliotheken | alle 7 Dateien vorhanden | **anhalten**, fehlende nennen |
| `plugins/scenegraph` | enthält `libqsgepaper.so` | **anhalten** |
| `plugins/platforms` | enthält `libepaper.so`; fehlt sie, nutzen die Apps automatisch die mitgelieferte | weiter, notieren |
| Eingabegeräte | Wacom-Digitizer (Stift), Touchscreen, Tasten; Namen und `eventN` notieren | weiter, Liste berichten |
| xochitl | `active` | **anhalten** |
| xovi | `LD_PRELOAD=/home/root/xovi/xovi.so` | fehlt es: **anhalten**, berichten |

## B2. Zeitmarke setzen und Hello-World installieren

Zuerst die Zeitmarke für die Prüfung in B8b. Sie ist der erste Schreibschritt und liegt in
`/home/root/apps`:
```sh
rM# mkdir -p /home/root/apps && touch /home/root/apps/.teststart && ls -la /home/root/apps/.teststart
PC$ deploy/install.sh hello-remarkable-rm1.tar.gz
```
**Erwartet:**
- `Installiert: 0.3.2 in /home/root/apps/hello-remarkable`.
- Die Liste zeigt `hello_remarkable`, `start.sh`, `fallback/`, `licenses/`, `VERSION`, **Besitzer
  `root root`** (in v0.3.1 war es `1001`).
- `rM# df -k /home` zeigt etwa 250 KB weniger frei.

## B3. xochitl stoppen (eigener Schritt)

> **Achtung (R12):** Ab jetzt ist das Tablet **unbedienbar**, bis xochitl in B5 wieder läuft. Das
> letzte Bild bleibt stehen, nichts reagiert – das ist **kein Absturz**. Im ersten Test sah es
> zweimal so aus, weil xochitl über eine Pause gestoppt blieb. B3 → B4 → B5 nur am Stück und nur,
> wenn der Besitzer am Gerät ist.

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
  wird benutzt (vgl. B1). Im ersten Test war das System-Plugin vorhanden.
- Die bekannten Meldungen aus A3 sind harmlos.

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
- Dann die **xovi-Prüfung aus A6**: `LD_PRELOAD=/home/root/xovi/xovi.so` ist **erwartet**.
- AppLoad und touch-lock sind wieder aktiv, das ist kein Fehler.

## B6. App installieren (PC)

```sh
PC$ deploy/install.sh remarkable-sp-rm1-0.3.2.tar.gz
rM# du -sk /home/root/apps/remarkable-sp
```
**Erwartet:** `Installiert: 0.3.2 in /home/root/apps/remarkable-sp`, Besitzer `root root`, rund
2,5 MB belegt.

## B7. xochitl stoppen, App starten und prüfen

Wie B3: ab hier ist das Tablet bis B8 unbedienbar, nur am Stück (R12).

```sh
rM# systemctl stop xochitl; systemctl is-active xochitl
rM# RMSP_LATENCY_LOG=1 /home/root/apps/remarkable-sp/start.sh
```
**Erwartet im Terminal:**
`Pen digitizer /dev/input/eventN (x 0..20967, y 0..15725, pressure 0..4095), transform swapxy,invy`

**Erwartet auf dem Bildschirm** (wie `soll-bild-rm1-0.3.2.png`): Oberfläche über die **volle
Fläche** – Icon-Leiste am linken Rand von oben bis unten, „Heute“ oben, der runde ＋-Knopf **am
rechten Rand oben**, Fokus/Sync/Einstellungen unten links.

**Objektiv prüfen (nur lesen, eigener Schritt):** während die App läuft, in einer zweiten Sitzung
```sh
rM# cat /sys/class/graphics/fb0/virtual_size /sys/class/graphics/fb0/bits_per_pixel /sys/class/graphics/fb0/stride
PC$ ssh <tablet> 'cat /dev/fb0' > fb0-b7.raw
PC$ python3 -I deploy/fb0-to-png.py fb0-b7.raw fb0-b7.png 1404 1872 <stride> <bits_per_pixel>
```
**Erwartet:** `fb0-b7.png` entspricht dem Soll-Bild; die Ausgabe nennt dunkle Pixel im rechten
Viertel **> 0 %** (＋-Knopf). Ist das Bild verrauscht oder verschoben, stimmen Format oder Größe
nicht: Werte aus `/sys` berichten, nicht weiter probieren. Das Gerät schreibt dabei nichts.

**Fenster zu klein** (wie in v0.3.1 nur ein Viertel): anhalten, Abzug und Terminal-Ausgabe
berichten.

Mit dem Besitzer prüfen und je Punkt ja/nein berichten:

| # | Aktion | Erwartet |
|---|---|---|
| a | ＋ mit dem Finger antippen | Notizbuchseite öffnet sich, links die Werkzeugleiste |
| b | Mit dem Stift oben links ins Titelfeld schreiben | Tinte erscheint **direkt unter der Stiftspitze** |
| c | Mit dem Radierer-Ende über die Tinte | Tinte verschwindet |
| d | Handballen auf das Papier legen | keine Striche |
| e | ⌨ antippen, „Test“ tippen, Fertig; dann mit dem Stift ins Titelfeld schreiben | Getippter Titel steht groß da, die Handschrift ist ausgeblendet, Hinweis „Handschrift-Titel ausgeblendet …“; **der Stift malt nicht über den getippten Titel** |
| e2 | getippten Text wieder löschen (⌫), Fertig | Handschrift-Titel erscheint wieder |
| f | „Schließen“ unten links | zurück zur Liste, die Aufgabe ist sichtbar (mit Handschrift-Titel) – **im ersten Test nicht geprüft** |
| g | Aufgabe lange drücken | Kontextmenü erscheint – **im ersten Test nicht geprüft** |
| h | Latenz messen, siehe unten | `LATENZ`-Zeilen im Terminal |

**h – Latenz messen.** Der Besitzer zeichnet auf einer Notizseite nacheinander: (1) eine
langsame gerade Linie, (2) eine **schnelle Spirale**, (3) ein handgeschriebenes Wort. Nach jedem
Strich erscheint im Terminal eine Zeile
`LATENZ roh=… angekommen=… dichte=…% evt_app_ms[…] evt_canvas_ms[…] evt_frame_ms[…] frames=… render_ms[…] malen_us_je_punkt=…`,
beim Beenden `LATENZ-SUMME …`.
- `dichte`: Anteil der Stiftpunkte, die auf der Zeichenfläche ankommen (Soll ≥ 95 %).
- `evt_frame_ms`: Zeit vom Stiftereignis bis zum fertig gerenderten Bild. Die E-Ink-Auffrischung
  danach (am rM1 gemessen DU 320–410 ms) ist darin **nicht** enthalten.
- Dazu der Eindruck des Besitzers: Verzögerung, runde oder eckige Linien, Vergleich mit xochitl.
- **Optional:** Handy-Zeitlupe (240 fps) eines schnellen Strichs, Bilder bis zur Tinte zählen.
- **Erlaubte Vergleichsmessung:** App beenden und einmal mit Kantenglättung wie in v0.3.1 starten,
  dieselben drei Striche:
  `rM# RMSP_LATENCY_LOG=1 RMSP_INK_ANTIALIAS=1 /home/root/apps/remarkable-sp/start.sh`.
  Der Besitzer sagt, ob sich die Verzögerung spürbar unterscheidet.

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

**Erlaubte Varianten, wenn (a) nicht stimmt** (Tipp landet woanders): `start.sh` setzt als
Vorgabe `rotate=180` (im ersten Test richtig). Nur wenn das nicht passt: vor den Start
`QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="rotate=0"` setzen, dann `rotate=90`, dann `rotate=270`, und
berichten, welcher Wert stimmt.

**Beenden** mit **Strg+C**, dann:
```sh
rM# pidof remarkable-sp || echo "beendet"
rM# ls -la /home/root/apps/remarkable-sp/data
```
**Erwartet:** `beendet`; im Terminal zuerst `Signal 2: beende geordnet`, dann die Zeile
`LATENZ-SUMME …`; in `data/` liegen `sp-state.json`, `sp-client.json` und `ink/`.
`settings.ini` gibt es nur, wenn in diesem Lauf eine Einstellung geändert wurde (z. B.
Stiftbreite in c/h) – sie wird sofort beim Ändern geschrieben; fehlt sie ohne Änderung, ist das
kein Fehler. Auch ein Abbruch der SSH-Verbindung (SIGHUP) beendet die App geordnet.

## B8. xochitl starten (eigener Schritt)

Wie B5, einschließlich der xovi-Prüfung.

## B8b. Prüfen: keine Dateien außerhalb von `/home/root/apps` (nur lesen)

```sh
rM# find / -xdev -newer /home/root/apps/.teststart -type f 2>/dev/null
rM# find /home/root -xdev -path /home/root/.local -prune -o -newer /home/root/apps/.teststart -type f -print 2>/dev/null | grep -v '^/home/root/apps/'
rM# df -k /; du -sk /var/log/journal
```
**Erwartet:**
- Die erste Liste (Root-Dateisystem) enthält nur Dateien, die das System selbst schreibt:
  `/var/log/journal/…`, `/var/lib/systemd/random-seed` und **`/etc/passwd`, `/etc/shadow` samt
  `-`-Kopien** (xochitl schreibt sie bei jedem Start neu, im ersten Test sekundengenau belegt).
  Jeden Eintrag nennen, nicht als Fehler werten, nichts löschen.
- Die zweite Liste enthält nichts, was von der App stammt (erwartbar: memfault, xochitl-Cache und
  -Config, rm-delayed-hacks).
- Freier Platz auf `/`: Abnahme höchstens Wachstum von `/var/log/journal` seit B1 plus 1 MB (R7).
- Meldet BusyBox `find` eine unbekannte Option: Meldung berichten, nicht umbauen.

## B9. Rückbau (PC), nur auf Wunsch des Besitzers

Den Besitzer fragen: App behalten (für weitere Tests oder später für AppLoad) oder entfernen?

```sh
PC$ deploy/uninstall.sh hello-remarkable
PC$ deploy/uninstall.sh remarkable-sp                          # Programm weg, data/ bleibt
PC$ deploy/uninstall.sh remarkable-sp root@10.11.99.1 --purge  # alles weg
rM# rm -f /home/root/apps/.teststart; rmdir /home/root/apps 2>/dev/null; ls -la /home/root/apps 2>/dev/null || echo "apps-Ordner entfernt"; df -h / /home
```
**Erwartet:**
- „Entfernt: …“ bzw. „Programm entfernt, Daten behalten“.
- Nach vollständigem Rückbau ist der apps-Ordner entfernt.
- `/`: wie in B8b (Journal-Wachstum plus 1 MB), `/home` entspricht etwa B1.

## B10. Abschluss

Erst prüfen:
```sh
rM# systemctl is-active xochitl; pidof remarkable-sp hello_remarkable || echo "keine App aktiv"
```
**Erwartet:** `active`, `keine App aktiv`.

Dann die **xovi-Prüfung aus A6**. **Erwartet:** `LD_PRELOAD=/home/root/xovi/xovi.so`; AppLoad
ist sichtbar (Besitzer fragen).

**Nur wenn `LD_PRELOAD` fehlt**, und nur nach Freigabe des Besitzers, xovi neu laden. Das Skript
startet xochitl selbst neu:
```sh
rM# /home/root/xovi/start
```
Danach die xovi-Prüfung wiederholen.

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
   - die Terminal-Ausgaben aus B4 und B7, **alle `LATENZ`-Zeilen** (mit und ohne Vergleichsmessung),
   - der Bildschirmabzug `fb0-b7.png` mit der Ausgabe von `fb0-to-png.py`,
   - falls nötig der passende `RMSP_PEN_TRANSFORM`- und Touch-Wert,
   - der Eindruck zur Geschwindigkeit und zu den Linien (B7 h).
4. **Endzustand:**
   - Backup-Ordner `rm1-2026-10-08-0131`.
   - xochitl aktiv, `LD_PRELOAD` vorhanden.
   - Ergebnis von B8b.
   - App installiert oder entfernt.
   - Freier Platz auf `/` (B1 → Ende) und `/home`.
5. **Kein Passwort** im Bericht (R11).
