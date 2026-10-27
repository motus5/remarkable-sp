# Teststrategie reMarkable SP – Auswertung des Gerätetests v0.3.1 und Plan für die nächste Runde

Stand 10.10.2026. Grundlage: `BERICHT-RM1.md`, Code-Lesung (HEAD `1863018`), Befunde der Prüfung vom 10.10.
Lokale Datei, nicht committet.

## 1. Was wir gemacht haben – Einordnung

| Ebene | Vorhanden | Läuft wo | Bewertung |
|---|---|---|---|
| Unit | `tests/tst_core.cpp`, 12 Tests: Stroke, Canvas (zeichnen/radieren/undo/speichern), Stift-Transformation, Tablet-Events, Versionen, Codec + Verschlüsselung (inkl. Datei von Super Productivity), Store, Sync (Rundlauf, Konflikt, Ablehnung) | CI, `QT_QPA_PLATFORM=offscreen` | gut für Kern und Sync |
| Integration | ARM-Emulation (qemu) mit SDK-Bibliotheken | laut Doku, nicht in CI | unklar, was geprüft wird |
| Oberfläche (QML) | **keine Tests** | – | Lücke |
| Paket/Release | SHA256 | Release-Workflow | Besitzer, Pfade, Größe ungeprüft |
| Gerät | manueller Abnahmetest TEST-RM1.md, Besitzer als einziges „Auge“ | rM1 über WLAN, 3 Tage | hat die wichtigsten Fehler gefunden |

Der Gerätetest war das Einzige, was die Hauptfehler sichtbar gemacht hat. **Drei davon hätten schon in CI auffallen können** (Abschnitt 2). Nur die Stift-Latenz ist wirklich gerätegebunden.

## 2. Welcher Befund auf welche Ebene gehört

| Befund am Gerät | Ursache (Code-Lesung) | Früheste Ebene, die ihn findet |
|---|---|---|
| Oberfläche nur ein Viertel groß | `qml/Main.qml:7`: Fenster fest 702×936, Vollbild greift unter epaper nicht | **CI**: QML-Test mit Bildschirm 1404×1872 – Fensterbreite = Bildschirmbreite |
| Getippter Titel unter Tinte verborgen, Sync sendet anderen Titel | `qml/TaskDetail.qml:207` / Zeile 30 | **CI**: QML-Test Titel-Modi |
| Dateien gehören uid 1001 | `device/package-rm1.sh:50` ohne `--owner=0` | **CI**: Archiv-Prüfung `tar -tvzf` |
| Stift ~400 ms verzögert | vermutlich Antialiasing + Painted-Item + Wellenform (nicht gemessen) | **nur Gerät**; Teil davon messbar im Code (Punktdichte, Malzeit) |
| Linien eckig bei schnellem Zeichnen | vermutlich zu wenige Stützpunkte (nicht gemessen) | **CI** teilweise: Punktdichte; Wahrnehmung nur Gerät |
| TEST-RM1.md: `IMG_VERSION` falsch, `/etc/passwd`/`shadow` fehlen in B8b, 1-MB-Toleranz | Anleitung | Durchsicht gegen die gemessenen Gerätefakten (BERICHT Anhang B1) |
| Tablet „hängt“ (xochitl gestoppt) | Ablauf | Anleitung: B3–B5/B7–B8 am Stück |

## 3. Lücken des Gerätetests selbst

1. **B7 f und g nicht geprüft** (Schließen → Liste mit Handschrift-Titel; Kontextmenü).
2. **Latenz nur als Eindruck** („ca. 400 ms“) – nicht reproduzierbar, nicht vergleichbar zwischen Releases.
3. **Ohne Besitzer am Gerät keine Prüfung möglich**; ein Bild-Abzug über `/dev/fb0` (nur lesen) ist in TEST-RM1.md nicht vorgesehen.
4. **Vollbild nicht objektiv geprüft** – nur per Foto.
5. **Protokolle überschreiben sich** bei Wiederholung (`.testhilfen/lauf`).
6. **`/`-Toleranz** über Tage nicht haltbar; Journalgröße wurde nicht in B1 mitgemessen.
7. **Keine Langzeitprobe:** Was passiert mit laufender App über eine Stunde (Speicher, Ruhezustand)? Die Nacht vom 09./10.10. bleibt ungeklärt.

## 4. Plan für die nächste Runde (Release nach v0.3.1)

### 4.1 CI – neue Tests (Entwickler-Instanz)

| # | Test | Art | Prüft | Erwartung |
|---|---|---|---|---|
| T1 | `windowFillsScreen` | QML (`QQuickTest` oder C++ mit `QQmlApplicationEngine`, offscreen, `QT_QPA_OFFSCREEN_*`/Screen 1404×1872, `RMSP_FULLSCREEN=1`) | Größe des Hauptfensters | `width == Screen.width && height == Screen.height`; ＋-Knopf rechts von `0.9 * width` |
| T2 | `titleModesExclusive` | QML | Tippen und Tinte im Titelfeld | Nach Tippen ist kein Tinten-Titel sichtbar/aktiv (oder umgekehrt); was gespeichert und gesynct wird, ist das, was zu sehen ist |
| T3 | `penPointDensity` | C++ (vorhandenes `penDeliversTabletEvents` erweitern) | 200 Roh-Events in einem Schwung | im Canvas kommen ≥ 95 % als Stützpunkte an |
| T4 | `inkIsMonochrome` (falls Antialiasing abgeschaltet wird) | C++ | Pixel eines gezeichneten Strichs | nur Schwarz/Weiß, keine Graustufen |
| T5 | `inkPaintCost` | C++ Benchmark (`QBENCHMARK`) | Zeit je Stiftpunkt bis `update()` | Richtwert festlegen und als Regression überwachen |
| T6 | `releaseArchive` | Shell im Release-Workflow | `tar -tvzf` der Archive | Besitzer `0/0`, ein Oberordner, keine absoluten Pfade, kein `..`, keine Symlinks, Größe ≤ Grenzwert |

### 4.2 Emulation (qemu, SDK-Bibliotheken)
- App mit `-platform offscreen` und Bildschirm 1404×1872 starten, `QQuickWindow::grabWindow()` als PNG ablegen, T1 als Bildvergleich (Leiste links, ＋ rechts oben). Als Artefakt im Release anhängen, damit der Tester ein Soll-Bild hat.

### 4.3 Gerät – verkürztes Protokoll (Test-Instanz am ASUS, Besitzer am Gerät)

**Grundsatz:** Ein Block, Besitzer durchgehend am Gerät, xochitl nie über eine Pause gestoppt.

| Block | Inhalt | Dauer |
|---|---|---|
| V | Vorab nur lesen (B1) **plus** `du -sk /var/log/journal`; Akku; Uptime | ohne Besitzer |
| I | Installation (B2, B6) – xochitl läuft dabei normal | ohne Besitzer |
| G | B3 → B4 → B5 und B3 → B7 (a–h, **inkl. f, g**) → B8 am Stück | ~15 min, Besitzer da |
| N | B8b, B9 nach Wunsch, B10 | ohne Besitzer, AppLoad-Frage am Ende |

Neu in Block G:
1. **Objektive Vollbild-Prüfung:** nach dem App-Start `/dev/fb0` einmal **lesen** und am ASUS zu PNG wandeln (Gerät schreibt nichts). Muss als erlaubter Schritt in TEST-RM1.md stehen. Abnahme: Inhalt reicht bis an den rechten und unteren Rand.
2. **Latenz messbar machen:** entweder (a) App protokolliert bei `RMSP_LATENCY_LOG=1` Zeit vom Roh-Event bis zum Abschluss der Auffrischung (Entwickler), oder (b) Besitzer filmt einen schnellen Strich mit dem Handy in Zeitlupe (240 fps) und wir zählen Bilder. Ergebnis als Zahl im Bericht, Vergleich mit xochitl im selben Video.
3. **Strichqualität:** Besitzer zeichnet eine schnelle Spirale; Foto in den Bericht.
4. **Kurze Langzeitprobe (optional, nur mit Besitzer in Rufweite):** App 30 min laufen lassen, alle 5 min `pidof` und RSS (`/proc/<pid>/status`) lesen.

### 4.4 Abnahmekriterien für das nächste Release
| Kriterium | Messung | Soll |
|---|---|---|
| Vollbild | T1 grün + fb0-Abzug am Gerät | Inhalt bis an alle Ränder |
| Stift trifft | B7 b | Tinte unter der Spitze ohne Variante |
| Latenz | 4.3.2 | Richtwert vom Entwickler festlegen; muss besser als 400 ms sein |
| Linien | 4.3.3 + T3 | keine sichtbaren Ecken bei schnellem Strich |
| Titel | T2 + B7 e/f | gesehener Titel = gespeicherter Titel |
| Rückbau | B8b, B10 | keine Dateien außerhalb `/home/root/apps`, xochitl mit xovi |
| Paket | T6 | Besitzer root, Pfade sauber |

## 5. Verbesserungen an Anleitung und Werkzeug

**TEST-RM1.md (Entwickler):**
- B1: `IMG_VERSION="3.26.0.68"` / `VERSION_ID=5.6.75` erwarten.
- B1 und B8b: Journalgröße mitmessen; `/`-Toleranz auf Testdauer beziehen.
- B3: Warnung „Tablet ab jetzt unbedienbar, bis B5/B8 – nur am Stück, Besitzer am Gerät“.
- B7: Vorgabe `rotate=180` nennen; Fallunterscheidung „Fenster zu klein“ ergänzen.
- B8b: `/etc/passwd`, `/etc/shadow` und `-`-Kopien als Systemeinträge aufführen (xochitl schreibt sie bei jedem Start).
- Abschnitt „Test über nicht-interaktiven Kanal“: screen + `ssh -t`, Strg+C per `screen -X stuff`, WLAN-Vorkehrungen.

**Test-Instanz (am ASUS):**
- `.testhilfen/lauf`: Protokolle mit Zeitstempel im Namen, nie überschreiben.
- Jeder Geräteschritt beginnt mit Ping und endet mit `systemctl is-active xochitl` – so fällt ein offener Teststand sofort auf.
- Vor jeder längeren Pause: Zustand „xochitl aktiv, keine App“ herstellen.

## 6. Was bewusst nicht getestet wird
- WebDAV-Sync am Gerät (eigener Test, wenn die Oberfläche steht; in CI gut abgedeckt).
- Updater, Export, Einstellungen – erst nach Behebung von Vollbild und Latenz.
- Alles, was Schreiben außerhalb `/home/root/apps` bräuchte.
