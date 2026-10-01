<h1 align="center">reMarkable SP</h1>

<p align="center">
  <b>Super Productivity für das reMarkable – nativ, mit Stift und Zwei-Wege-Sync.</b><br>
  Aufgaben, Zeiterfassung und Planung auf E-Ink, handschriftlich wie im Notizbuch.
</p>

<p align="center">
  <a href="https://github.com/motus5/remarkable-sp/actions/workflows/ci.yml"><img src="https://github.com/motus5/remarkable-sp/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <img src="https://img.shields.io/badge/reMarkable-1%20(2%20folgt)-black" alt="reMarkable 1">
  <img src="https://img.shields.io/badge/Qt-6%20%2F%20QML-41cd52" alt="Qt 6">
  <img src="https://img.shields.io/badge/Lizenz-MIT-blue" alt="MIT">
</p>

<table>
  <tr>
    <td><img src="docs/liste.png" alt="Aufgabenliste"></td>
    <td><img src="docs/notizbuch.png" alt="Aufgabe als Notizbuchseite"></td>
    <td><img src="docs/navigation.png" alt="Seitenleiste"></td>
  </tr>
  <tr>
    <td align="center">Heute-Liste</td>
    <td align="center">Aufgabe als Notizbuchseite</td>
    <td align="center">Projekte &amp; Tags</td>
  </tr>
  <tr>
    <td><img src="docs/kontextmenue.png" alt="Kontextmenü"></td>
    <td><img src="docs/vollbild.png" alt="Vollbild mit Punktraster"></td>
    <td><img src="docs/einstellungen.png" alt="Einstellungen"></td>
  </tr>
  <tr>
    <td align="center">Lange drücken = Menü</td>
    <td align="center">Vollbild-Papier</td>
    <td align="center">Sync &amp; Updates</td>
  </tr>
</table>

## Funktionen

- **Wie [Super Productivity](https://github.com/johannesjo/super-productivity):**
  - Heute-Liste, Projekte mit Backlog, Tags, Unteraufgaben
  - Planen (Heute/Morgen/nächste Woche), Zeitschätzung, Priorität
  - Zeiterfassung pro Tag, Fokus-Timer, Arbeitsprotokoll, Markdown-Export
- **Stift:**
  - Titel und Notizen handschriftlich, mit Druckstufen, Radierer-Spitze, Rückgängig und Wiederholen
  - Notizpapier liniert, kariert, gepunktet oder leer
  - Nur der Stift schreibt, der Finger bedient
- **Wie das reMarkable:**
  - Icon-Leiste und Seitenleiste wie in der Bibliothek
  - Werkzeugleiste zum Einklappen, Popover-Menüs
  - Blättern per Wischen, Bildschirmtastatur
  - Keine Animationen, kontrastreich für E-Ink
- **Zwei-Wege-Sync:**
  - Mit Super Productivity auf Computer und Handy über WebDAV, Nextcloud oder einen Ordner
  - Auch komprimiert und Ende-zu-Ende-verschlüsselt
- **Updates in der App** aus den GitHub-Releases, mit Prüfsumme

## Installation auf dem reMarkable

> **Status:** Am Linux-Desktop getestet, Sync gegen WebDAV und das Format von Super Productivity 19.
> Die rM1-Version ist mit dem offiziellen SDK gebaut und unter Emulation geprüft, auf echter
> Hardware aber **noch nicht erprobt** (siehe [TEST-RM1.md](TEST-RM1.md)).

| Gerät | Release-Datei | Stand |
|---|---|---|
| reMarkable 1 (Firmware 3.26.0.68) | `remarkable-sp-rm1-<version>.tar.gz` | gebaut, Gerätetest offen |
| reMarkable 2 | – | folgt (eigenes SDK) |

Die App nutzt das Qt 6 des Systems und das epaper-Backend von reMarkable
([offizielle Anleitung](https://developer.remarkable.com/documentation/qt_epaper)).
Alles liegt in **`/home/root/apps/<app>`**; nichts kommt nach `/usr`, `/etc` oder `/opt`, und es
gibt keinen Autostart.

1. **SSH:** Gerät per USB verbinden. Das Passwort steht unter *Einstellungen → Hilfe → Copyrights und
   Lizenzen*.
2. **Installieren** vom Rechner aus, im Ordner dieses Repos:
   ```sh
   deploy/install.sh remarkable-sp-rm1-0.3.1.tar.gz        # nach /home/root/apps/remarkable-sp
   ```
3. **Starten:**
   ```sh
   ssh root@10.11.99.1
   systemctl stop xochitl
   /home/root/apps/remarkable-sp/start.sh                   # Ende mit Strg+C
   systemctl start xochitl
   ```
4. **Entfernen:** `deploy/uninstall.sh remarkable-sp`. Die Daten bleiben erhalten, mit `--purge`
   wird alles gelöscht.

Der Start über AppLoad ist geplant. Updates gibt es in der App unter *Einstellungen → Nach Updates
suchen*.

## Verwendung

| Aktion | So geht’s |
|---|---|
| Neue Aufgabe | runder **＋**-Knopf oben rechts, dann den Titel auf die Linie schreiben |
| Erledigt | Kästchen antippen |
| Zeit erfassen | **▶** in der Zeile oder in der Werkzeugleiste |
| Blättern | mit dem Finger hoch/runter wischen (Liste) bzw. links/rechts (Aufgaben) |
| Menü zu einer Aufgabe | Aufgabe **lange drücken** |
| Planen, Schätzung, Projekt, Tags | Chips unter dem Titel antippen |
| Stiftbreite, Vorlage | Stift- bzw. Vorlagen-Symbol in der Werkzeugleiste |
| Vollbild-Papier | runder Knopf oben links |
| Tippen statt schreiben | ⌨-Symbol, dann öffnet sich die Bildschirmtastatur |

### Sync mit Super Productivity einrichten

1. In Super Productivity (ab v19) den Sync einrichten: **WebDAV/Nextcloud** oder
   **Lokale Datei**. Dann einmal synchronisieren. „Surgical sync“ muss aus sein.
2. Auf dem reMarkable unter *Einstellungen*: dieselbe WebDAV-Adresse, Benutzer, Passwort und
   denselben Sync-Ordner eintragen (Standard `super-productivity`), dazu ggf. das
   Verschlüsselungs-Passwort.
3. Fertig. Die App synchronisiert beim Start, kurz nach Änderungen und im eingestellten
   Intervall. **↻** in der Leiste startet den Sync von Hand.

Handschrift bleibt auf dem Gerät. Nur handschriftlich angelegte Aufgaben heißen in
Super Productivity „✍ Handschrift (reMarkable)“.

## Weiterentwickeln

Die App ist in C++/Qt 6 und QML geschrieben. Entwickelt wird bequem am Linux-PC, dort spielt die
Maus den Stift.

```sh
sudo apt install qt6-base-dev qt6-base-private-dev qt6-declarative-dev qml6-module-qtquick \
  qml6-module-qtquick-window qml6-module-qtqml-workerscript zlib1g-dev libssl-dev cmake g++

cmake -S . -B build && cmake --build build -j
./build/remarkable-sp                          # Fenster in halber rM-Größe
RMSP_OSK=1 ./build/remarkable-sp               # mit Bildschirmtastatur
QT_QPA_PLATFORM=offscreen ./build/rmsp_tests   # Tests
```

| Pfad | Inhalt |
|---|---|
| `src/spstore.*` | Datenmodell im Format von Super Productivity, Änderungen als SP-Operationen |
| `src/syncengine.*`, `src/syncbackend.*`, `src/syncfile.*` | Sync (WebDAV/Ordner), Konflikte, gzip, Verschlüsselung |
| `src/workspace.*` | Heute/Projekte/Tags, Zeiterfassung, Fokus, Protokoll |
| `src/inkcanvas.*`, `src/icon.*` | Zeichenfläche und Linien-Icons |
| `src/updater.*` | Update aus GitHub-Releases |
| `qml/` | Oberfläche (`Theme.qml` = Farben, Maße) |
| `deploy/` | Installation, Launcher-Dateien, Release-Upload |

**Für das rM1 bauen** mit dem offiziellen SDK
([Download-Liste](https://developer.remarkable.com/links), passend zur Firmware):
```sh
sh remarkable-production-image-5.6.75-rm1-public-x86_64-toolchain.sh -y -d ~/sdk-rm1
EPAPER_QPA=~/epaper-qpa device/package-rm1.sh ~/sdk-rm1 dist-rm1   # epaper-qpa optional
```
Den Stift liest `src/peninput.cpp` direkt vom Wacom-Eingabegerät, weil das epaper-Plugin nur Touch
verarbeitet.

**Release:** Version in `CMakeLists.txt` erhöhen und Tag `vX.Y.Z` pushen. `release.yml` baut die
Desktop-Version und die rM1-Pakete mit dem SDK und hängt beides an das Release.

Mitmachen: siehe [CONTRIBUTING.md](CONTRIBUTING.md). Fehler und Ideen bitte als
[Issue](https://github.com/motus5/remarkable-sp/issues).

## Roadmap

- [x] Geräte-Build für rM1 in der CI (offizielles SDK)
- [ ] Test auf rM1, dann rM2-Build
- [ ] Start über AppLoad
- [ ] Aufgaben umsortieren
- [ ] Wiederkehrende Aufgaben und Notizen aus SP anzeigen
- [ ] Handschrifterkennung (Tinte → Text)
- [ ] „Surgical sync“ (SP-Format v3), Dropbox

## Lizenz

[MIT](LICENSE). Enthält die [Argon2-Referenzimplementierung](third_party/argon2) (CC0/Apache-2.0).
Nicht offiziell mit reMarkable AS oder Super Productivity verbunden.
