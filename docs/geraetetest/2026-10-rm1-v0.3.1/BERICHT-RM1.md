# Bericht Gerätetest reMarkable 1 – reMarkable SP v0.3.1

| | |
|---|---|
| Test-Instanz | Test-Instanz auf dem ASUS (<Test-PC>, Nutzer `<Nutzer>`) |
| Besitzer / Beobachter | Besitzer (alle Bildschirm-Beobachtungen wörtlich von ihm, R4) |
| Repo-Stand | `feature/remarkable-sp-app`, HEAD `186301862f0e89b90ec66e935c07dd1ea8b414a1` |
| Release | v0.3.1 (`hello-remarkable-rm1.tar.gz`, `remarkable-sp-rm1-0.3.1.tar.gz`) |
| Zeitraum | 08.10.2026 09:51 UTC (Zeitmarke) bis 10.10.2026 ~10:25 UTC |
| Ort der Rohdaten | ASUS: `<lokaler Projektordner>/.testhilfen/` (`schritte/` = jedes ausgeführte Skript, `log/` = jede Ausgabe) |

Kein Passwort in diesem Bericht (R11).

## 1. Kurzfazit

| Frage | Ergebnis |
|---|---|
| Hello-World läuft | **ja** – Bild, Text, Fingertipp schalten das Quadrat |
| App läuft | **ja** – startet, Liste und Notizbuchseite erscheinen, Daten werden geschrieben |
| Stift richtig | **ja** – Tinte unter der Spitze mit `transform swapxy,invy` (Vorgabe, keine Variante nötig); **aber ~400 ms Verzögerung, bei schnellem Zeichnen eckig** |
| Touch richtig | **ja** – Fingertipp trifft (＋ öffnet Seite); Vorgabe `rotate=180` aus start.sh passt, keine Variante nötig |
| Hauptbefund | **Oberfläche viel zu klein**: nutzt nur das linke obere Viertel – Fenster fest 702×936 (`qml/Main.qml:7`, siehe 4.1.1) |
| Nicht geprüft | B7 f (Schließen → Liste) und g (Kontextmenü) |

## 2. Abweichungen vom Testplan (vom Besitzer entschieden)

1. **WLAN statt USB** (`root@<Tablet-IP im WLAN>` statt `10.11.99.1`). USB-Daten kommen am ASUS nicht an: `usb 3-1` meldete am 08.10. 01:48–04:24 184 gescheiterte Anmeldungen (`device descriptor read/64, error -71`, `device not accepting address`), nie eine Gerätekennung → Kabel ohne Datenleitung oder Micro-USB-Buchse. Netzwerkkonfiguration des ASUS nicht beteiligt (nur gelesen, nichts geändert). Das Tablet lädt über das Kabel.
2. **Passwort per sshpass**, nur als Umgebungsvariable in der screen-Sitzung (ausdrückliche Ausnahme des Besitzers). Steht in keiner Datei.
3. **Alle Befehle in der screen-Sitzung `rm1test` auf dem ASUS**, je Schritt ein Skript; Strg+C per `screen -X stuff`. Apps mit `ssh -t` gestartet (Hinweis DEVCloud). Daneben Ping-Schleife `rm1keepalive`.
4. **Host-Argument** an `deploy/install.sh` / `uninstall.sh`.
5. **Klon** in `<lokaler Projektordner>` (lokale ext4-Platte des ASUS) statt `~/src`.
6. **`df -k`** zusätzlich zu `df -h` in B2/B6/B9 – `df -h` zeigt bei 6 GB keine 0,3 MB.
7. **Verlauf über drei Tage** mit zwei Unterbrechungen (Abschnitt 6).
8. B7 f/g vom Besitzer übersprungen, Rückbau vollständig (`--purge`).
## 3. Schritte B0–B10

Vollständige Terminal-Ausgaben stehen in Anhang A (unverändert aus `.testhilfen/log/`).

| Schritt | Ergebnis | Erwartung erfüllt |
|---|---|---|
| **B0** Dateien/SHA256 | beide Archive `OK`; `install.sh`/`uninstall.sh` schreiben nur unter `/home/root/apps`; Archive: ein Oberordner, keine absoluten Pfade, kein `..`, keine Symlinks | ja |
| **B0b** Backup | `<Backup-Ordner des Besitzers> rm1-2026-10-08-0131` (42/42, geprüft von der Chat-Instanz; für die Test-Instanz nicht lesbar). Nicht erneut ausgeführt | ja |
| **B1** Vorab | Uptime 2 d 21 h; `IMG_VERSION="3.26.0.68"`, `VERSION_ID=5.6.75`; Kernel `5.4.70-v1.6.2-rm10x armv7l`; `/` 97 % (5,6 MB); `/home` 6,0 GB; Akku 100 %; alle 7 Qt-Bibliotheken (6.8.2); `libqsgepaper.so`, `libepaper.so` vorhanden; event0 Wacom, event1 gpio-keys, event2 cyttsp5_mt; xochitl `active`; kein apps-Ordner; `LD_PRELOAD=/home/root/xovi/xovi.so` | ja, bis auf `IMG_VERSION` (siehe 4.1) |
| **B2** Zeitmarke + Hello-World | `.teststart` 08.10. 09:51:17 UTC; `Installiert: 0.3.1 in /home/root/apps/hello-remarkable`; alle 5 Einträge; `/home` −256 KB; `/` unverändert 5 766 KB (Referenz R7) | ja |
| **B3** xochitl stoppen | `inactive` | ja |
| **B4** Hello-World | Besitzer: Rahmen + „Hello reMarkable 1“ **ja**, Zeile 1404×1872 + Quadrat **ja**, Fingertipp schaltet Quadrat **ja**. Kein „bundled epaper plugin“ (System-Plugin genutzt). Strg+C beendet, `pidof` leer | ja |
| **B5** xochitl starten | `keine App aktiv`, `active`, `LD_PRELOAD` vorhanden; Besitzer: Oberfläche und AppLoad **ja** | ja |
| **B6** App installieren | `Installiert: 0.3.1 in /home/root/apps/remarkable-sp`; `du` 2 476 KB; `/` unverändert | ja |
| **B7** App | Terminal: `Pen digitizer /dev/input/event0 (x 0..20967, y 0..15725, pressure 0..4095), transform swapxy,invy` | ja |
| B7 Bild | Besitzer: Icon-Leiste, „Heute“, ＋ **ja** – „aber viel zu klein“ (Foto 1) | teilweise |
| B7 a ＋ antippen | **ja** (Foto 2: Notizbuchseite mit Werkzeugleiste) | ja |
| B7 b Stift | **ja**, Tinte unter der Spitze (Foto 2; geschrieben ins Linienfeld, Titelfeld nicht separat bestätigt) | ja |
| B7 c Radierer | **ja** | ja |
| B7 d Handballen | **ja**, keine Striche | ja |
| B7 e Tastatur | Besitzer wörtlich: „geht aber es kann auch mit dem stift reingemalt werden und wen man das erst wieder radiert erscheint der text den man getippt hat“ | teilweise |
| B7 f Schließen | **nicht geprüft** (Pause über Nacht, siehe 6) | – |
| B7 g Kontextmenü | **nicht geprüft** | – |
| B7 h Geschwindigkeit | Besitzer wörtlich: „erscheint während des zeichnens und mit ca 400ms verzögerung und teilweise eckick statt runde linien... aber eher wenn man schnell zeichnet. auf jedenfall kein vergleich zu der orginal zeichnen app vom rm.“ | – (Eindruck) |
| B7 Ende | Um 10.10. 10:16 UTC lief keine App mehr; `data/` zuletzt 09.10. 22:02 UTC geschrieben: `sp-state.json`, `sp-client.json`, `settings.ini`, `ink/` (1 Datei, 28 KB). **Zeitpunkt und Ursache des Endes nicht belegt** – vermutlich Hangup beim Abriss der SSH-Sitzung durch den ASUS-Neustart 10.10. 05:29 Ortszeit; ein vorheriger Hänger der App ist nicht auszuschließen (siehe 6.2) | teilweise |
| **B8** xochitl starten | `keine App aktiv`, `active`, `LD_PRELOAD` vorhanden; Besitzer: bedienbar, AppLoad **ja** | ja |
| **B8b** Dateien | Liste 1 (`/`): 6 Journal-Dateien, `random-seed`, `/etc/passwd(-)`, `/etc/shadow(-)` – alle vom System (siehe 4.4). Liste 2: 152 Einträge, nur memfault, xochitl-Cache/-Config, rm-delayed-hacks; kein App-Bezug | ja |
| B8b `/` | 4 485 KB statt 5 766 KB = −1 281 KB, **über Toleranz 1 MB**; `/var/log/journal` 7,5 MB, App schrieb nichts auf `/` | nein (System) |
| **B9** Rückbau | `--purge` auf Wunsch: beide entfernt, `.teststart` gelöscht, apps-Ordner entfernt; `/home` 6 266 680 KB | ja |
| **B10** Abschluss | 10.10. 10:29 UTC: xochitl `active`, `keine App aktiv`, `LD_PRELOAD=/home/root/xovi/xovi.so`, `00-xovi.conf` vorhanden, kein apps-Ordner. AppLoad: vom Besitzer nach B8 bestätigt, seither xochitl nicht mehr neu gestartet. `/home/root/xovi/start` nicht nötig | ja |
## 4. Für den Entwickler

### 4.1 Befunde zur App (nach Gewicht)
1. **Oberfläche zu klein – Ursache gefunden (Code-Lesung 10.10.).** Die App belegt nur das linke obere **Viertel** (halbe Breite, halbe Höhe; der „runde Knopf oben rechts“ sitzt auf dem Foto etwa in Bildschirmmitte). `qml/Main.qml:7` setzt das Fenster fest auf 702×936 und verlässt sich auf `visibility: Window.FullScreen`; unter dem epaper-Plugin bleibt es bei 702×936, alle Maße hängen über `u = width / 100` daran. Gegenprobe: `device/hello/Main.qml` setzt `width: Screen.width; height: Screen.height` und lief vollflächig. Behebung: im Vollbildfall Bildschirmgröße setzen.
2. **Stift-Latenz ~400 ms, eckige Linien bei schnellem Zeichnen** (Besitzer). Tinte erscheint während des Zeichnens, nicht erst danach. Deutlich schlechter als xochitl.
3. **Tastatur-Feld (B7 e):** Stift-Tinte lässt sich über getippten Text malen und verdeckt ihn bis zum Radieren. Vermutlich sollten Handschrift- und Tastatur-Modus sich ausschließen.
4. **Dateibesitz:** Archive tragen uid/gid **1001** – nach `tar -x` als root gehören die Dateien auf dem Gerät einem nicht existierenden Nutzer. Vorschlag: Archiv mit `--owner=0 --group=0` bauen oder `tar -xzo`.
5. Terminal-Meldungen in B4 und B7 (Bild trotzdem korrekt, ungeklärt):
   `Could not open bin file for reading` (3×), `rm.epaperkeyboardhandler: Failed to open pogo lang status` / `No keymap … defaulting to US` (je 2×), `QFSFileEngine::map: Mapping a file beyond its size is not portable`.
6. Entpackte Größe der App 2,4 MB (`du` 2 476 KB), TEST-RM1.md erwartet „rund 2,7 MB“ – nur Doku.

### 4.2 Befunde zu TEST-RM1.md
1. **B1 `IMG_VERSION`:** Erwartung `IMG_VERSION="5.6.75"` ist falsch. Gerät: `IMG_VERSION="3.26.0.68"`, `VERSION_ID=5.6.75`.
2. **B8b Liste 1:** `/etc/passwd`, `/etc/shadow` und ihre `-`-Kopien gehören zu den Einträgen, die das System selbst schreibt (4.4) – in die Erwartung aufnehmen.
3. **R7 / `/`-Toleranz 1 MB** hält über mehrere Tage nicht: das Journal wächst von selbst (siehe 4.4). Vorschlag: Toleranz auf die Testdauer beziehen oder `/var/log/journal` in B1 und B8b mitmessen.
4. **Zwischen B3 und B5/B8 ist das Tablet unbedienbar** (letztes Bild steht, nichts reagiert). Für den Besitzer sieht das wie ein Absturz aus – siehe 6. Ein deutlicher Hinweis gehört in B3, dazu: B3–B5 und B7–B8 nur am Stück, Besitzer am Gerät.
5. B0 nennt `git clone`, der in Auftrag Schritt 0 schon erledigt ist – Doppelung.
6. Strg+C setzt ein interaktives Terminal voraus. Für eine nicht-interaktive Test-Instanz: `screen` + `ssh -t` dokumentieren.

### 4.3 Werte, die nicht verändert werden mussten
- `RMSP_PEN_TRANSFORM`: Vorgabe `swapxy,invy` passt.
- `QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS`: keine Variante nötig.
- epaper-Plugin: System-`libepaper.so` vorhanden und benutzt, Fallback nicht nötig.

### 4.4 Systemverhalten (kein Testschaden, belegt)
- **xochitl schreibt bei jedem Start `/etc/passwd` und `/etc/shadow` neu.** Zeitstempel auf die Sekunde gleich mit den Starts in B5 (09.10. 20:54:12 UTC → `passwd-`/`shadow-`) und B8 (10.10. 10:17:13 UTC → `passwd`/`shadow`). Erklärt, warum ein selbst gesetztes Root-Passwort früher „nicht gehalten“ hat.
- `/var/log/journal` am Ende 7,5 MB (Wachstum seit B1 nicht gemessen); **vermutlich** Hauptursache für den Schwund auf `/` (Juni 8,5 MB → 07.10. 6,9 MB → 08.10. 5,8 MB → 10.10. 4,5 MB frei).
- `.memfault/mar/` sammelt ~145 Manifest-Dateien in zwei Tagen.
- Timer `rm-delayed-hacks`: `systemctl` nannte den Lauf am 08.10. „1 day 9h ago“, das Datum (05.10. 12:39 UTC) passte nicht zur Uptime – ungeklärt, ohne Auswirkung.
## 5. Endzustand

| | |
|---|---|
| Backup | `rm1-2026-10-08-0131` (unverändert, nicht erneut ausgeführt) |
| xochitl | `active`, `LD_PRELOAD=/home/root/xovi/xovi.so` (B8, 10.10. 10:17 UTC); Besitzer: bedienbar, AppLoad sichtbar. B10 bestanden (10:29 UTC) |
| B8b | keine Dateien vom Test außerhalb `/home/root/apps` |
| App | **entfernt** (`--purge`), `/home/root/apps` gelöscht |
| `/` frei | 5 766 KB (B1/B2) → 4 484 KB (B9) = −1 282 KB, Ursache Systemjournal |
| `/home` frei | 6 268 596 KB (B2) → 6 266 680 KB (B9); Differenz außerhalb `apps` (xochitl-Cache, memfault) |
| Notizbücher | nicht angefasst (`/home/root/.local/share/remarkable` nie gelesen) |

## 6. Verlauf und Erkenntnisse zur Arbeitsweise

### 6.1 Verlauf
| Zeit (Ortszeit) | Ereignis |
|---|---|
| 08.10. 11:45–11:52 | Schritt 0/1, B0–B3. xochitl gestoppt |
| 08.10. nachmittags | Besitzer nicht am Gerät → B4 vertagt. **xochitl blieb gestoppt** |
| 09.10. früh | Besitzer: Tablet „hing komplett“, Neustart per Taste (Uptime danach passt) |
| 09.10. 22:50–22:56 | B3–B7 (B4 und B7 mit Besitzer am Gerät) |
| 09.10. Nacht | Pause **mit laufender App**. Besitzer am Morgen: „hat sich aufgehängt“ |
| 10.10. 05:29 | ASUS neu gestartet (Besitzer) → screen-Sitzungen weg, SSH-Hangup beendet die App sauber |
| 10.10. 12:16–12:30 | Zustandsprüfung: Tablet **nicht** neu gestartet, xochitl `inactive` → B8, B8b, B9, B10 |

### 6.2 Erkenntnisse
1. **„Tablet hängt“ hieß zweimal: xochitl war gestoppt.** Ohne xochitl gibt es keine Oberfläche; das letzte Bild bleibt stehen, nichts reagiert. Ob die App in der zweiten Nacht vor 05:29 selbst hing, lässt sich nicht belegen – sie schrieb nichts mehr ins Terminal, `data/` wurde zuletzt 22:02 UTC geschrieben. **Fehler der Test-Instanz:** den Teststand über Nacht offen gelassen und den Besitzer nicht deutlich genug gewarnt.
   → Regel künftig: **xochitl nur stoppen, wenn der Besitzer am Gerät ist; B3→B4/B7→B5/B8 am Stück; nie über eine Pause.**
2. **Bildschirm aus der Ferne:** rm-shot/quicksettings-screenshot sind xovi-Erweiterungen in xochitl – bei gestopptem xochitl nicht verfügbar. Ohne Besitzer am Gerät sind Bild- und Touch-Prüfung nicht möglich. Fotos des Besitzers haben die Bewertung deutlich verbessert.
3. **Nicht-interaktiver Kanal:** Strg+C, Passwortabfragen und `ssh -t` gehen nur über `screen` auf dem ASUS. Bewährt: je Schritt ein Skript in `.testhilfen/schritte/`, Ausgabe nach `.testhilfen/log/` mit Endmarke; Passwort nur als Umgebungsvariable der screen-Sitzung, Bash-Historie dort aus.
4. **screen überlebt keinen Neustart des ASUS.** Bei laufender App beendet der Abriss sie (dank `ssh -t` / Hangup) – das ist gut, aber der Teststand ist danach offen (xochitl aus). Nach jedem ASUS-Neustart: Sitzungen neu anlegen und **zuerst den Gerätezustand lesen**.
5. **Zeitstempel sind das beste Diagnosewerkzeug.** Die Änderung an `/etc/passwd`/`shadow` ließ sich über `stat` auf die Sekunde den xochitl-Starts zuordnen, ohne den Inhalt zu lesen.
6. **WLAN statt USB ging stabil:** Tablet am Ladekabel, Ping-Schleife, `ServerAliveInterval=10`; kein einziger Verbindungsabbruch durch das Tablet. USB scheitert an Kabel/Buchse (Daten), nicht am ASUS.
7. **MCP-Kanal ans ASUS:** LAN-Zugang zeitweise nicht erreichbar, WiFi-Zugang nimmt höchstens 7 000 Zeichen je Befehl → längere Dateien in Teilen schreiben.
8. **Eigene Irrtümer, berichtigt:** geschätzte Uhrzeit im ersten Ist-Stand; Timer-Erklärung „Uhr nachgestellt“ war nur Vermutung; „SSH-Verbindung zur App weggebrochen“ war der ASUS-Neustart; „Tablet hing“ war xochitl aus.

## Anhang A – Terminal-Ausgaben (unverändert)

Hinweis: B3 lief zweimal (08.10. und 09.10.); das Protokoll des ersten Laufs wurde vom zweiten überschrieben, hier steht nur der Lauf vom 09.10. B8b gekürzt: 145 Zeilen `/home/root/.memfault/mar/<uuid>/manifest.json` zusammengefasst.

### b0
```
curl SHA256SUMS rc=0
curl hello-remarkable-rm1.tar.gz rc=0
curl remarkable-sp-rm1-0.3.1.tar.gz rc=0
-rw-rw-r-- 1 <Nutzer> <Nutzer>   79682  8. Okt 11:46 hello-remarkable-rm1.tar.gz
-rw-rw-r-- 1 <Nutzer> <Nutzer> 1003835  8. Okt 11:46 remarkable-sp-rm1-0.3.1.tar.gz
-rw-rw-r-- 1 <Nutzer> <Nutzer>     275  8. Okt 11:46 SHA256SUMS
--- SHA256SUMS ---
84a646b2b122ebac08010b8ef20ff6953ec8ee890b39e357fa9a3b17e6b68b70  hello-remarkable-rm1.tar.gz
1458c37d00cc852fa4dda1eb1f691b67f2bbe70324ea2a88bd9c1da8a1318edc  remarkable-sp-rm1-0.3.1.tar.gz
2169bd1ce1ab32bce94ce3ea7b83c37f0b5bc23860d1b7939f85840c1c06bfe8  remarkable-sp-rm1
--- Pruefung ---
hello-remarkable-rm1.tar.gz: OK
remarkable-sp-rm1-0.3.1.tar.gz: OK
--- install.sh ---
#!/bin/sh
# Installs a release archive on the reMarkable into /home/root/apps/<app>.
#   deploy/install.sh <archive.tar.gz> [ssh-host]     (default host: root@10.11.99.1)
# Writes nothing outside /home/root/apps, starts nothing, no autostart.
# An existing data/ folder of the app is kept.
set -eu
ARCHIVE=${1:?Archiv angeben, z. B. remarkable-sp-rm1-0.3.1.tar.gz}
HOST=${2:-root@10.11.99.1}
APP=$(tar -tzf "$ARCHIVE" | head -1 | cut -d/ -f1)
case "$APP" in
    remarkable-sp|hello-remarkable) ;;
    *) echo "Unbekanntes Archiv (Ordner '$APP')" >&2; exit 1 ;;
esac
NEED_KB=$(( $(gzip -l "$ARCHIVE" | awk 'NR==2 {print $2}') / 1024 * 2 + 1024 ))

echo "Installiere $APP nach $HOST:/home/root/apps/$APP (benötigt ca. ${NEED_KB} KB)"
ssh "$HOST" "set -e
    FREE=\$(df -k /home | awk 'NR==2 {print \$4}')
    case \"\$FREE\" in ''|*[!0-9]*) echo 'Freier Platz in /home nicht ermittelbar' >&2; exit 1 ;; esac
    if [ \"\$FREE\" -lt $NEED_KB ]; then echo \"Zu wenig Platz in /home: \$FREE KB frei\" >&2; exit 1; fi
    mkdir -p /home/root/apps"
scp "$ARCHIVE" "$HOST:/home/root/apps/.$APP-install.tar.gz"
ssh "$HOST" "set -e
    cd /home/root/apps
    tar -xzf .$APP-install.tar.gz
    rm -f .$APP-install.tar.gz
    chmod +x $APP/start.sh
    echo \"Installiert: \$(cat $APP/VERSION 2>/dev/null) in /home/root/apps/$APP\"
    ls -la /home/root/apps/$APP"
--- uninstall.sh ---
#!/bin/sh
# Removes an app from /home/root/apps on the reMarkable.
#   deploy/uninstall.sh <remarkable-sp|hello-remarkable> [ssh-host] [--purge]
# Without --purge the app's data/ folder (tasks, handwriting, settings) is kept.
set -eu
APP=${1:?App angeben: remarkable-sp oder hello-remarkable}
HOST=${2:-root@10.11.99.1}
PURGE=${3:-}
case "$APP" in
    remarkable-sp|hello-remarkable) ;;
    *) echo "Unbekannte App '$APP'" >&2; exit 1 ;;
esac
ssh "$HOST" "set -e
    DIR=/home/root/apps/$APP
    [ -d \"\$DIR\" ] || { echo \"\$DIR existiert nicht – nichts zu tun\"; exit 0; }
    if pidof remarkable-sp hello_remarkable >/dev/null 2>&1; then
        echo 'App läuft noch – bitte zuerst beenden' >&2; exit 1
    fi
    if [ '$PURGE' = '--purge' ] || [ ! -d \"\$DIR/data\" ]; then
        rm -rf \"\$DIR\"
        echo \"Entfernt: \$DIR\"
    else
        for f in \"\$DIR\"/* \"\$DIR\"/.[!.]*; do
            [ -e \"\$f\" ] || continue
            [ \"\${f##*/}\" = data ] && continue
            rm -rf \"\$f\"
        done
        echo \"Programm entfernt, Daten behalten: \$DIR/data\"
    fi
    rmdir /home/root/apps 2>/dev/null || true"
== ENDE rc=0 ==
```

### b0b-archive
```
=== hello-remarkable-rm1.tar.gz ===
Eintraege: 10
Oberste Ordner: hello-remarkable 
Absolute Pfade: 0
Pfade mit '..': 0
Symlinks:
  keine
Gesamtgroesse entpackt: 0.2 MB
Erste Ebene:
  hello-remarkable/fallback
  hello-remarkable/hello_remarkable
  hello-remarkable/licenses
  hello-remarkable/start.sh
  hello-remarkable/VERSION
=== remarkable-sp-rm1-0.3.1.tar.gz ===
Eintraege: 12
Oberste Ordner: remarkable-sp 
Absolute Pfade: 0
Pfade mit '..': 0
Symlinks:
  keine
Gesamtgroesse entpackt: 2.4 MB
Erste Ebene:
  remarkable-sp/fallback
  remarkable-sp/licenses
  remarkable-sp/remarkable-sp
  remarkable-sp/start.sh
  remarkable-sp/VERSION
== ENDE rc=0 ==
```

### b1
```
	1352974.rm1keepalive	(08.10.2026 11:49:09)	(Detached)
4 packets transmitted, 4 received, 0% packet loss, time 3005ms
rtt min/avg/max/mdev = 3.644/4.134/5.346/0.704 ms
### uptime
 09:49:13 up 2 days, 21:19,  load average: 0.40, 0.44, 0.49
### timer
-                              - Mon 2026-10-05 12:39:18 UTC 1 day 9h ago rm-delayed-hacks.timer       rm-delayed-hacks.service
### os-release
ID=codex
NAME="Codex Linux"
VERSION="5.6.75 (scarthgap)"
VERSION_ID=5.6.75
VERSION_CODENAME="scarthgap"
PRETTY_NAME="Codex Linux 5.6.75 (scarthgap)"
CPE_NAME="cpe:/o:openembedded:codex:5.6.75"
BUILD_MODE_RM="public"
IMG_VERSION="3.26.0.68"
### uname
Linux reMarkable 5.4.70-v1.6.2-rm10x #1 Thu Aug 21 08:18:59 UTC 2025 armv7l GNU/Linux
### df
Filesystem                Size      Used Available Use% Mounted on
/dev/root               228.2M    206.7M      5.6M  97% /
/dev/mmcblk1p7            6.5G    201.3M      6.0G   3% /home
### update.conf
REMARKABLE_RELEASE_VERSION=3.26.0.68
### akku
100
### qt-bibliotheken
lrwxrwxrwx    1 root     root            19 Mar 10  2026 /usr/lib/libQt6Core.so.6 -> libQt6Core.so.6.8.2
lrwxrwxrwx    1 root     root            18 Mar 10  2026 /usr/lib/libQt6Gui.so.6 -> libQt6Gui.so.6.8.2
lrwxrwxrwx    1 root     root            22 Mar 10  2026 /usr/lib/libQt6Network.so.6 -> libQt6Network.so.6.8.2
lrwxrwxrwx    1 root     root            18 Mar 10  2026 /usr/lib/libQt6Qml.so.6 -> libQt6Qml.so.6.8.2
lrwxrwxrwx    1 root     root            20 Mar 10  2026 /usr/lib/libQt6Quick.so.6 -> libQt6Quick.so.6.8.2
-rwxr-xr-x    1 root     root       3169812 Mar 10  2026 /usr/lib/libcrypto.so.3
lrwxrwxrwx    1 root     root            13 Mar 10  2026 /usr/lib/libz.so.1 -> libz.so.1.3.1
### plugins
/usr/lib/plugins/platforms/:
libepaper.so
libqminimal.so
libqoffscreen.so
libqvnc.so

/usr/lib/plugins/scenegraph/:
libqsgepaper.so
### eingabegeraete
N: Name="Wacom I2C Digitizer"
H: Handlers=event0 
N: Name="gpio-keys"
H: Handlers=kbd event1 
N: Name="cyttsp5_mt"
H: Handlers=event2 
### xochitl
active
### apps
kein apps-Ordner
### xovi
LD_PRELOAD=/home/root/xovi/xovi.so
### xochitl.service.d
00-xovi.conf
extensions.d
exthome
qt-resource-rebuilder.conf
== ENDE rc=0 ==
```

### b2
```
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 2.444/3.347/3.886/0.642 ms
### vorher: df -k / /home
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    211626      5766  97% /
/dev/mmcblk1p7         6845496    206120   6268596   3% /home
### Zeitmarke
-rw-r--r--    1 root     root             0 Oct  8 09:51 /home/root/apps/.teststart
### install.sh
Installiere hello-remarkable nach root@<Tablet-IP im WLAN>:/home/root/apps/hello-remarkable (benötigt ca. 1484 KB)
Installiert: 0.3.1 in /home/root/apps/hello-remarkable
drwxr-xr-x    4 1001     1001          4096 Oct  8 09:51 .
drwxr-xr-x    3 root     root          4096 Oct  8 09:51 ..
-rw-r--r--    1 1001     1001             6 Oct  1 05:57 VERSION
drwxr-xr-x    3 1001     1001          4096 Oct  8 09:51 fallback
-rwxr-xr-x    1 1001     1001         30496 Oct  1 05:57 hello_remarkable
drwxr-xr-x    2 1001     1001          4096 Oct  8 09:51 licenses
-rwxr-xr-x    1 1001     1001           860 Oct  1 05:57 start.sh
install.sh rc=0
### nachher: df -k / /home
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    211626      5766  97% /
/dev/mmcblk1p7         6845496    206376   6268340   3% /home
== ENDE rc=0 ==
```

### b3
```
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 2.109/3.041/3.614/0.665 ms
stop rc=0
inactive
kein xochitl-Prozess
== ENDE rc=0 ==
```

### b4
```
Start Asus-Zeit 22:50:42
Could not open bin file for reading
Could not open bin file for reading
Could not open bin file for reading
rm.epaperkeyboardhandler: Failed to open pogo lang status:  "No such file or directory"
rm.epaperkeyboardhandler: No keymap set by QT settings or firmware, defaulting to US.
rm.epaperkeyboardhandler: Failed to open pogo lang status:  "No such file or directory"
rm.epaperkeyboardhandler: No keymap set by QT settings or firmware, defaulting to US.
QFSFileEngine::map: Mapping a file beyond its size is not portable
^CConnection to <Tablet-IP im WLAN> closed.
ssh/start.sh rc=255
Ende Asus-Zeit 22:52:22
== ENDE rc=0 ==
```

### b5
```
### R6
keine App aktiv
### start
start rc=0
active
### xovi
LD_PRELOAD=/home/root/xovi/xovi.so
== ENDE rc=0 ==
```

### b6
```
### vorher
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212907      4485  98% /
/dev/mmcblk1p7         6845496    207816   6266900   3% /home
Installiere remarkable-sp nach root@<Tablet-IP im WLAN>:/home/root/apps/remarkable-sp (benötigt ca. 5924 KB)
Installiert: 0.3.1 in /home/root/apps/remarkable-sp
drwxr-xr-x    4 1001     1001          4096 Oct  9 20:55 .
drwxr-xr-x    4 root     root          4096 Oct  9 20:55 ..
-rw-r--r--    1 1001     1001             6 Oct  1 05:57 VERSION
drwxr-xr-x    3 1001     1001          4096 Oct  9 20:55 fallback
drwxr-xr-x    2 1001     1001          4096 Oct  9 20:55 licenses
-rwxr-xr-x    1 1001     1001       2282044 Oct  1 05:57 remarkable-sp
-rwxr-xr-x    1 1001     1001           921 Oct  1 05:57 start.sh
install.sh rc=0
### nachher
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212907      4485  98% /
/dev/mmcblk1p7         6845496    210292   6264424   3% /home
2476	/home/root/apps/remarkable-sp
== ENDE rc=0 ==
```

### b7
```
stop rc=0
inactive
App-Start Asus-Zeit 22:56:02
Could not open bin file for reading
Could not open bin file for reading
Could not open bin file for reading
rm.epaperkeyboardhandler: Failed to open pogo lang status:  "No such file or directory"
rm.epaperkeyboardhandler: No keymap set by QT settings or firmware, defaulting to US.
rm.epaperkeyboardhandler: Failed to open pogo lang status:  "No such file or directory"
rm.epaperkeyboardhandler: No keymap set by QT settings or firmware, defaulting to US.
Pen digitizer /dev/input/event0 (x 0..20967, y 0..15725, pressure 0..4095), transform swapxy,invy
QFSFileEngine::map: Mapping a file beyond its size is not portable
Connection to <Tablet-IP im WLAN> closed.
tcsetattr: Input/output error
tcsetattr: Input/output error
```

### z2
```
### uptime
 10:16:11 up 1 day,  2:05,  load average: 0.00, 0.01, 0.00
### xochitl
inactive
### app
keine App aktiv
### xovi
kein LD_PRELOAD
### apps
drwxr-xr-x    4 root     root          4096 Oct  9 20:55 .
drwx------   16 root     root          4096 Oct  8 09:51 ..
-rw-r--r--    1 root     root             0 Oct  8 09:51 .teststart
drwxr-xr-x    4 1001     1001          4096 Oct  8 09:51 hello-remarkable
drwxr-xr-x    5 1001     1001          4096 Oct  9 20:56 remarkable-sp
### data
/home/root/apps/remarkable-sp/data:
drwxr-xr-x    3 root     root          4096 Oct  9 22:02 .
drwxr-xr-x    5 1001     1001          4096 Oct  9 20:56 ..
drwxr-xr-x    2 root     root          4096 Oct  9 22:02 ink
-rw-r--r--    1 root     root            25 Oct  9 22:02 settings.ini
-rw-r--r--    1 root     root           941 Oct  9 22:01 sp-client.json
-rw-r--r--    1 root     root           826 Oct  9 22:01 sp-state.json

/home/root/apps/remarkable-sp/data/ink:
drwxr-xr-x    2 root     root          4096 Oct  9 22:02 .
drwxr-xr-x    3 root     root          4096 Oct  9 22:02 ..
-rw-r--r--    1 root     root         28797 Oct  9 22:00 dP0_AjVCh9pOfLj2vfJrs-notes.json
### df
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212907      4485  98% /
/dev/mmcblk1p7         6845496    210720   6263996   3% /home
== ENDE rc=0 ==
```

### b8
```
### R6
keine App aktiv
### start
start rc=0
active
### xovi
LD_PRELOAD=/home/root/xovi/xovi.so
== ENDE rc=0 ==
```

### b8b
```
### Liste 1: / neuer als .teststart
/var/log/journal/53e61e8c1462421999113a69d24b3046/system@87cc6b9429f44549a5ca5bf3fb094fb6-000000000008460c-00065d6d8b4699d1.journal
/var/log/journal/53e61e8c1462421999113a69d24b3046/system@87cc6b9429f44549a5ca5bf3fb094fb6-000000000008627f-00065d6e3e34f11c.journal
/var/log/journal/53e61e8c1462421999113a69d24b3046/system@87cc6b9429f44549a5ca5bf3fb094fb6-0000000000086b48-00065d6e8ca7929c.journal
/var/log/journal/53e61e8c1462421999113a69d24b3046/system.journal
/var/log/journal/53e61e8c1462421999113a69d24b3046/system@87cc6b9429f44549a5ca5bf3fb094fb6-00000000000858e7-00065d6e03540703.journal
/var/log/journal/53e61e8c1462421999113a69d24b3046/system@87cc6b9429f44549a5ca5bf3fb094fb6-0000000000084f59-00065d6dca215486.journal
/var/lib/systemd/random-seed
/etc/shadow-
/etc/passwd-
/etc/passwd
/etc/shadow
### Liste 1 Ende (rc=0)
### Liste 2: /home/root ohne .local und apps
/home/root/.cache/remarkable/xochitl/.eventcache.db-wal
/home/root/.cache/remarkable/xochitl/.eventcache.db-shm
/home/root/.cache/remarkable/xochitl/qmlcache/cbd699404e4bc286d886d3434e8d4a0167a6cd0c.qmlc
/home/root/.cache/remarkable/xochitl/qmlcache/a1803fa0ff52c209efe6916c0a5c41480cfacb03.qmlc
/home/root/.memfault/last_tracked_boot_id
/home/root/.config/rm-delayed-hacks/last-run.log
/home/root/.config/remarkable/xochitl.conf
### Liste 2 Ende
### df
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212907      4485  98% /
== ENDE rc=0 ==
```

### b8b-zeit
```
2026-10-10 10:17:13.000000000 +0000 /etc/passwd
2026-10-09 20:54:12.000000000 +0000 /etc/passwd-
2026-10-10 10:17:13.000000000 +0000 /etc/shadow
2026-10-09 20:54:12.000000000 +0000 /etc/shadow-
2026-10-08 09:51:17.209200754 +0000 /home/root/apps/.teststart

Sat Oct 10 10:23:19 UTC 2026
 10:23:19 up 1 day,  2:12,  load average: 0.36, 0.36, 0.18

7688	/var/log/journal
== ENDE rc=0 ==
```

### b9
```
Entfernt: /home/root/apps/hello-remarkable
rc=0
Entfernt: /home/root/apps/remarkable-sp
rc=0
apps-Ordner entfernt
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212908      4484  98% /
/dev/mmcblk1p7         6845496    208036   6266680   3% /home
== ENDE rc=0 ==
```

### b10
```
### xochitl
active
### app
keine App aktiv
### xovi
LD_PRELOAD=/home/root/xovi/xovi.so
### xochitl.service.d
00-xovi.conf
extensions.d
exthome
qt-resource-rebuilder.conf
### apps
kein apps-Ordner
### df
Filesystem           1K-blocks      Used Available Use% Mounted on
/dev/root               233673    212908      4484  98% /
/dev/mmcblk1p7         6845496    208036   6266680   3% /home
Sat Oct 10 10:29:49 UTC 2026
== ENDE rc=0 ==
```
