<h1 align="center">reMarkable SP</h1>

<p align="center">
  <b>Super Productivity für das reMarkable – nativ, mit Stift und Zwei-Wege-Sync.</b><br>
  Aufgaben, Zeiterfassung und Planung auf E-Ink, handschriftlich wie im Notizbuch.
</p>

<p align="center">
  <a href="https://github.com/motus5/remarkable-sp/actions/workflows/ci.yml"><img src="https://github.com/motus5/remarkable-sp/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <img src="https://img.shields.io/badge/reMarkable-1%20%7C%202-black" alt="reMarkable 1 | 2">
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

> **Status:** Die App läuft am Linux-Desktop und ist gegen WebDAV und das Sync-Format von
> Super Productivity 19 getestet. Auf echter reMarkable-Hardware ist sie noch **nicht erprobt**.
> Fertige Geräte-Builds erscheinen unter [Releases](https://github.com/motus5/remarkable-sp/releases).

| Gerät | Datei im Release | Hinweis |
|---|---|---|
| reMarkable 1 | `remarkable-sp-arm` | Framebuffer direkt nutzbar |
| reMarkable 2 | `remarkable-sp-arm` | braucht zusätzlich `rm2fb` bzw. `qtfb` |
| Paper Pro / Move | `remarkable-sp-arm64` | ungetestet |

1. **SSH aktivieren:** Gerät per USB verbinden. Das Root-Passwort steht unter
   *Einstellungen → Hilfe → Copyrights und Lizenzen*. Dann `ssh root@10.11.99.1`.
2. **Launcher installieren** (einmalig), z. B. über [Toltec](https://toltec-dev.org): *remux*,
   *Oxide* oder *draft*. Vorher prüfen, ob Toltec deine Firmware-Version unterstützt.
3. **App kopieren und eintragen**, am Computer im Ordner dieses Repos:
   ```sh
   deploy/install.sh remarkable-sp-arm      # kopiert nach /opt/bin + Launcher-Eintrag
   ```
4. Im Launcher **„Aufgaben (SP)“** starten.

Spätere Versionen installierst du direkt in der App unter
*Einstellungen → Nach Updates suchen*. Die vorherige Version bleibt als `.old` erhalten.

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
sudo apt install qt6-base-dev qt6-declarative-dev qml6-module-qtquick \
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

**Für das Gerät bauen:** mit einer Qt-6-Toolchain für ARM, z. B. mit dem reMarkable-SDK:
`cmake -S . -B build-rm -DCMAKE_TOOLCHAIN_FILE=<sdk>/toolchain.cmake -DRMSP_BUILD_TESTS=OFF`.
Eine fertige Build-Pipeline für das Gerät fehlt noch. Hilfe dabei ist sehr willkommen.

**Release:** Version in `CMakeLists.txt` erhöhen und Tag `vX.Y.Z` pushen. Dann den Geräte-Build mit
`deploy/release-device.sh vX.Y.Z <binary> arm` anhängen.

Mitmachen: siehe [CONTRIBUTING.md](CONTRIBUTING.md). Fehler und Ideen bitte als
[Issue](https://github.com/motus5/remarkable-sp/issues).

## Roadmap

- [ ] Geräte-Build in der CI und Test auf rM1/rM2
- [ ] Aufgaben umsortieren
- [ ] Wiederkehrende Aufgaben und Notizen aus SP anzeigen
- [ ] Handschrifterkennung (Tinte → Text)
- [ ] „Surgical sync“ (SP-Format v3), Dropbox

## Lizenz

[MIT](LICENSE). Enthält die [Argon2-Referenzimplementierung](third_party/argon2) (CC0/Apache-2.0).
Nicht offiziell mit reMarkable AS oder Super Productivity verbunden.
