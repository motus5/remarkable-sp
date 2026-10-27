# Learnings – reMarkable-Apps

Gilt für jede App in diesem Repo und für neue Apps am reMarkable, etwa Taschenrechner oder
Bild-zu-PDF. Gesammelt aus der Entwicklung von reMarkable SP und zwei Gerätetests (v0.3.1, v0.3.2).

- **Pflege:** Jede neue Falle kommt mit Datum und Beleg (Issue, Commit oder Bericht) dazu. Am besten
  im selben Commit wie die Korrektur.
- **Datenschutz:** Kein Passwort, keine Adressen, keine Personennamen.

## 1. Gerät (reMarkable 1)

- **System:** OS 3.26.0.68 (`VERSION_ID=5.6.75`), Kernel 5.4.70, armv7l, Qt 6.8.2 im System.
- **Bildschirm:** 1404 × 1872 px, 226 ppi.
- **Eingabe:** Stift event0 (0..20967 × 0..15725, Druck 0..4095, Transformation `swapxy,invy`),
  Tasten event1, Touch event2 (`rotate=180`).
- **Shell ist BusyBox:** `head -n 5`, kein `ps aux`, kein Komfort-`find`.
- **`/` ist zu 97–98 % voll** und das Journal wächst. Nie nach `/usr`, `/etc`, `/opt` oder `/lib`
  schreiben; Platz gibt es nur auf `/home`. Apps liegen unter `/home/root/apps/<app>` bzw.
  `/home/root/xovi/exthome/appload/<app>`.
- **xochitl** schreibt `/etc/passwd` und `/etc/shadow` bei jedem Start neu; das ist harmlos und kein
  Fehler.
- **Tabu:** rmfakecloud, der `/etc/hosts`-Block, prx-Connectoren und die Notizbuch-Ordner (Backup
  nur lesend).
- **`systemctl stop xochitl` ohne startende App** lässt das Tablet eingefroren aussehen. Der
  Besitzer hält das für einen Absturz (im Test zweimal). Deshalb xochitl nur stoppen, wenn der
  Besitzer am Gerät ist, und immer mit einem sichtbaren Weg zurück. Prüfen, ob die App als
  AppLoad-App gehen kann (siehe `docs/PRUEFUNG-START-OHNE-PC.md`).
- **E-Ink-Auffrischung** (gemessen): DU 320–410 ms, GC16 ≈ 810 ms. Die App-Latenz (~15 ms) ist dagegen
  klein; spürbare Verzögerung entsteht in der Auffrischung.
- **Test über WLAN:** Das USB-Kabel kann ohne Datenleitung sein. Bei einem nicht-interaktiven Kanal
  `screen` bzw. `ssh -t` nutzen (TEST-RM1.md A8).

## 2. Qt/QML auf dem Gerät

| Falle | Folge | Lösung | Beleg |
|---|---|---|---|
| `Window.FullScreen` greift unter epaper nicht | App nur ein Viertel groß | Größe auf `Screen.width/height` setzen | #3 |
| Das Offscreen-Plugin wendet FullScreen dagegen an | Vollbild-Test grün trotz Fehler | epaper simulieren (`RMSP_SIMULATE_EPAPER`), Gegenprobe | #3 |
| Ein Pointer-Handler bekommt ein Ereignis eines Geräts, das er nicht annimmt, und ruft `setActive(false)` | Handballen (Touch) bricht den Stiftstrich ab, nur 67 % der Punkte kommen an | Touch filtern, solange der Stift in Reichweite ist (`src/palmfilter.*`) | #11 |
| Am Gerät gibt es keine Ersatzschriften | ⇧, ⌫, ✓, ⚠, ≤, ✍ als leere Kästchen; am Desktop unsichtbar, weil Systemschriften einspringen | Nur Zeichen der mitgelieferten Schrift, Symbole als gezeichnete Icons; Test `glyphsInBundledFont` | #13 |
| Ganze Seite neu zeichnen (`rebuildBuffer` + `update()`) | Rückgängig und Radierer träge (am rM1 ~250 µs je Punkt), Auffrischung des ganzen Bildschirms | Nur den geänderten Bereich neu zeichnen und auffrischen | #12 |
| Graue Kantenglättung | Langsamere Graustufen-Auffrischung | Tinte am Gerät schwarz/weiß | #4 |
| Strg+C bzw. SIGHUP ohne Handler | Kein `aboutToQuit`, Summe fehlt, bis zu 1,5 s Tinte verloren | Self-Pipe für SIGINT/TERM/HUP, Zeichenfläche speichert beim Abbau | #14 |
| Sichtbarkeit an abgeleiteten Zustand gebunden, der sich beim Tippen ändert | Titelfeld verschwand beim ersten Buchstaben | Sichtbar, solange das Feld den Fokus hat; Oberfläche interaktiv testen | #13 |
| Feste Höhen in der Werkzeugleiste | Bei offener Tastatur überlappende oder angeschnittene Knöpfe | Nur zeigen, was ganz hineinpasst | #13 |
| Keine Möglichkeit zum Beenden in der Oberfläche | Ohne PC kein Weg zurück | „App beenden“ | #15 |

## 3. Build und Werkzeuge

- **Offizielles SDK** statt eigenem Qt-Build: `remarkable-production-image-5.6.75-rm1-public`.
- **64-Bit-`time_t` im rM1-SDK:** `input_event` hat kein Feld `time`; `input_event_sec`/`input_event_usec`
  nutzen.
- **Archive:** mit `tar --owner=0 --group=0 --numeric-owner` packen; v0.3.1 hatte uid 1001.
  `device/check-archive.sh` prüft das.
- **qemu-arm mit dem SDK-Sysroot:** ARM-Smoke-Test und Soll-Bild ohne Gerät, als Prüfschritt im
  Paketbau.
- **AutoMoc:** Hinter `signals:` dürfen nur Signale stehen, sonst kommt „Not a signal declaration“.
- **QVector:** Eine Referenz auf ein Element ist nach `append()` ungültig. Kopie nehmen.
- **ctest ohne Display:** `QT_QPA_PLATFORM=offscreen` per `set_tests_properties` setzen.
- **Tags pushen:** Der Proxy blockiert Tag-Pushes. Releases laufen über `workflow_dispatch` mit Input
  `tag`.
- **Xvfb/xdotool für Oberflächentests:**
  - `RMSP_SOFTWARE=1`, sonst bleibt das Bildschirmfoto schwarz.
  - Koordinaten sind Bildschirmpixel; Werte aus verkleinerten Bildern zurückrechnen.
  - Jede Bewegung als eigener Aufruf.

## 4. Testen und Messen

- **Jeder Test braucht eine Gegenprobe:** Er muss mit dem alten Code bzw. ohne die Korrektur rot
  werden. Mehrfach hat das falsch grüne Tests aufgedeckt: T1-Vollbild unter Offscreen, T8 mit
  SIGINT.
- **Shell-Hintergrundjobs ignorieren SIGINT.** Wer SIGINT testet, startet das Programm über einen
  Wrapper, der SIGINT zurücksetzt.
- **Messung prüfen, bevor man ihr glaubt:** Die Dichte zählte bei mehreren Punkten je Lesevorgang
  nur einen. Jetzt gilt eine Warteschlange je Punkt, gezählt wird je Stiftstrich. Am Desktop kommen
  Punkte einzeln, am Gerät gebündelt; deshalb gibt es die Wiedergabe-Variante `-burst`.
- **Gerätebefunde zuerst am Desktop nachstellen**, z. B. Handballen mit `RMSP_PEN_REPLAY=spiral-burst-palm`.
  Erst wenn der Fehler reproduziert ist, korrigieren.
- **„Fertig“ nur für Getestetes.** Was nur am Gerät prüfbar ist, als offen kennzeichnen; ein
  Desktop-Ergebnis ersetzt keinen Gerätetest.
- **Jede Dateiänderung rücklesen**; eine leere Ausgabe gilt als Fehlschlag.

## 5. Zusammenarbeit

- **GitHub ist der Kanal:**
  - Aufträge unter `docs/auftraege/`.
  - Befunde als Issues, Testberichte als PR.
  - Recherchen als PR mit Bericht unter `docs/recherche/`.
- **Autor aller Commits ist motus5**, ohne Co-Authored-By-Zeilen und ohne Werkzeug-Hinweise in
  Commits, PRs oder Dateien. Beiträge mit solchen Zeilen werden inhaltlich übernommen und neu
  committet, nicht gemergt.
- **Öffentliches Repo:** nie Passwörter, Heimnetz-Adressen, Rechner- oder Personennamen.
- **Angaben anderer Instanzen erst prüfen.** Eine Instanz prüfte einmal das falsche Repo.
- **Erst recherchieren, dann bauen:** Vorhandene Lösungen prüfen (Lizenz, Pflege, Eignung für rM1),
  Kurzbericht vorlegen, Freigabe abwarten.
- **Gerätetest:** nur mit dem Besitzer am Gerät, jeder Geräteschritt mit seinem „ok“, xochitl nur
  am Stück stoppen (TEST-RM1.md R12).
