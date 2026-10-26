# Bericht Gerätetest reMarkable 1 – reMarkable SP v0.3.2

| | |
|---|---|
| Datum | 26./27.10.2026, App-Lauf 00:20–00:43 Ortszeit |
| Gerät | reMarkable 1, `IMG_VERSION="3.26.0.68"`, `VERSION_ID=5.6.75`, Kernel 5.4.70 armv7l, Qt 6.8.2 |
| Release | v0.3.2 (Tag `9178808`), Prüfsummen OK, `check-archive.sh` (T6) ok für beide Archive |
| Verbindung | WLAN, Test-Instanz am PC des Besitzers, Besitzer am Gerät (R12 eingehalten) |
| Abweichung | Hello-World (B2-Installation, B3–B5) bewusst ausgelassen – lief in v0.3.1 einwandfrei, nur Archiv-Besitzer geändert. B8b/B9/B10 stehen noch aus (Gerät war anderweitig belegt) |

Kein Passwort, keine Adressen in diesem Bericht.

## 1. Kurzfazit

| Frage | Ergebnis |
|---|---|
| Vollbild (#3) | **behoben** – Bildschirmabzug = Soll-Bild (`bildschirmabzug-b7.png`), dunkle Pixel rechtes Viertel 1,84 % |
| Archiv-Besitzer (#6) | **behoben** – alle Dateien auf dem Gerät `root:root` |
| Titelfeld (#5) | **behoben** – Stift malt nicht über getippten Titel; nach Löschen kommt die Handschrift zurück |
| Stift-Latenz in der App (#4) | **behoben** – Stiftereignis bis fertiges Bild im Mittel 14,8 ms (46 Striche) |
| **Neu: Stift-Aussetzer** | **Hauptbefund** – nur 67 % der Stiftpunkte kommen auf der Zeichenfläche an |
| f Schließen / g Kontextmenü | **ja / ja** (im ersten Test offen) |

## 2. Schritte

| Schritt | Ergebnis | Erwartung |
|---|---|---|
| B0 | Prüfsummen OK; T6 ok: hello 10 Einträge 219 KB, App 12 Einträge 2471 KB, Besitzer 0/0 | ja |
| B1 | Uptime 16 h; alle 7 Qt-Bibliotheken; `libepaper.so`, `libqsgepaper.so`; event0 Wacom, event1 gpio-keys, event2 cyttsp5_mt; fb0 1408×3840, 16 bit, Stride 2816; xochitl `active`; `LD_PRELOAD` vorhanden; `/` 6 062 KB frei, `/var/log/journal` 6 111 KB | ja |
| B2 | nur Zeitmarke (`.teststart` 26.10. 23:19:15 UTC) | ja |
| B6 | `Installiert: 0.3.2`, `du` 2 508 KB, alle Dateien `root:root`, `/` unverändert | ja |
| B7 Start | `Pen digitizer /dev/input/event0 (x 0..20967, y 0..15725, pressure 0..4095), transform swapxy,invy`; sonst dieselben Meldungen wie v0.3.1 (`Could not open bin file` 3×, `pogo lang status` 2×, `QFSFileEngine::map`) | ja |
| B7 Bildschirmabzug | `fb0-to-png.py`: 1404×1872, rechtes Viertel 1,84 %, unteres Viertel 0,50 %; Bild deckungsgleich mit `soll-bild-rm1-0.3.2.png` | ja |
| 0 Volle Fläche | Besitzer: „ja“ | ja |
| a ＋ | „ja“ | ja |
| b Stift Titelfeld | „ja, da schreibt es. Aber mit Lücken. Diesmal anders. Die Linien bleiben rund, aber setzen aus.“ | teilweise |
| c Radierer | „Radierer macht das ganze Wort weg. Sofort.“ | ja (Strich-Radierer?) |
| d Handballen | keine Striche, aber „teilweise setzt dann der gesamte Schreibvorgang aus“ | teilweise |
| e Titel | Stift malt nicht über getippten Titel | ja |
| e2 | Rücktaste löscht, Handschrift-Titel erscheint wieder (Foto) | ja |
| f | Aufgabe in der Inbox, mit Handschrift-Titel | ja |
| g | Kontextmenü erscheint | ja |
| h | Linie, schnelle Spirale, Wort: „alles geht, aber Verzögerung ca. 3 mm hinter der bewegten Stiftspitze. Aussetzer während der Spirale mit Strichen. Die werden dann vom Display nachgezeichnet … wenn man das Tablet normal verwendet, nicht so.“ | teilweise |
| B7 Ende | Strg+C beendet, `pidof` leer. `data/`: `sp-state.json`, `sp-client.json`, `ink/` – **`settings.ini` fehlt**; **keine `LATENZ-SUMME`-Zeile** | teilweise |
| B8 | `keine App aktiv`, `active`, `LD_PRELOAD=/home/root/xovi/xovi.so` | ja |
| B8b, B9, B10 | ausstehend; App bleibt vorerst installiert (Wunsch Besitzer) | – |

## 3. Latenzmessung (`RMSP_LATENCY_LOG=1`, 46 Striche)

| Größe | Wert |
|---|---|
| Stiftpunkte roh / auf der Zeichenfläche | 14 459 / 9 688 = **67,0 %** (Soll ≥ 95 %) |
| Striche mit Dichte < 50 % | 11 von 46 (Einzelwerte bis 0,7 %) |
| Striche mit Dichte ≥ 95 % | u. a. 100 %, 99,5 % – Aussetzer sind also nicht dauerhaft |
| `evt_frame_ms` Mittel | 14,8 ms (typisch avg 9–12 ms, p95 10–33 ms, Ausreißer max 310 ms) |
| `evt_app_ms` / `evt_canvas_ms` | avg 2–4 ms / 3–5 ms |
| `malen_us_je_punkt` | 250–650 µs |

Lesart (nicht belegt): Die Punkte erreichen die App (`evt_app`), aber nicht die Zeichenfläche.
Zusammen mit Beobachtung d (Handballen → Schreiben setzt aus) liegt nahe, dass Touch-Ereignisse
den laufenden Stiftstrich unterbrechen oder der Strich an ein anderes Element geht. Die sichtbare
Verzögerung (~3 mm hinter der Spitze) entsteht nach dem fertigen Bild, also in der E-Ink-Auffrischung.

Kompletter Mitschnitt der `LATENZ`-Zeilen: Anhang A.

## 4. Neue Befunde

1. **Stift-Aussetzer** (siehe 3) – gestrichelte Linien, die das Display teils nachzeichnet. Hauptbefund.
2. **Handballen unterbricht das Schreiben** (d).
3. **Rückgängig reagiert verzögert** – „so dass man nicht schnell viele Linien wegmachen kann“.
4. **Bildschirmtastatur:** Umschalt ⇧ und Rücktaste ⌫ zeigen nur ein leeres Kästchen (Zeichen fehlt in der Geräteschrift, `qml/Keyboard.qml:67/70`); Rücktaste funktioniert, Umschalten nicht geprüft. Tastatur-Symbol erscheint doppelt (oben rechts und am Textfeld). Bei offener Tastatur überlappen „0m“ und „Schließen“ in der Werkzeugleiste.
5. **Beenden:** keine `LATENZ-SUMME`-Zeile (Strg+C über `ssh -t`); `settings.ini` fehlt in `data/`.
6. **Seitenanzeige** sprang von 1/1 auf 1/2, ohne dass der Besitzer bewusst eine Seite anlegte (zu klären).
7. **Radierer** löscht ganze Striche auf einmal – gewollt (Strich-Radierer)?
8. **Wunsch Besitzer:** App in der Vorlesung nutzen. Geht derzeit nicht ohne PC, weil xochitl per SSH gestoppt werden muss. Ein Start ohne PC (z. B. über AppLoad oder einen Startknopf) wäre nötig.

## Anhang A – LATENZ-Zeilen

```
LATENZ roh=308 angekommen=292 dichte=94.8% evt_app_ms[avg=4.2 p95=8.9 max=75.2] evt_canvas_ms[avg=2.9 p95=4.0 max=6.4] evt_frame_ms[avg=10.5 p95=21.6 max=84.9] frames=138 render_ms[avg=2.9 p95=1.9 max=93.5] malen_us_je_punkt=284.4
LATENZ roh=436 angekommen=397 dichte=91.1% evt_app_ms[avg=3.9 p95=3.9 max=82.9] evt_canvas_ms[avg=2.9 p95=4.3 max=7.4] evt_frame_ms[avg=11.2 p95=22.4 max=92.6] frames=185 render_ms[avg=2.2 p95=1.8 max=71.3] malen_us_je_punkt=254.8
LATENZ roh=47 angekommen=27 dichte=57.4% evt_app_ms[avg=2.8 p95=3.8 max=5.5] evt_canvas_ms[avg=6.5 p95=8.3 max=88.5] evt_frame_ms[avg=19.0 p95=94.3 max=97.9] frames=14 render_ms[avg=10.4 p95=71.0 max=71.0] malen_us_je_punkt=318.5
LATENZ roh=36 angekommen=35 dichte=97.2% evt_app_ms[avg=2.6 p95=4.8 max=6.6] evt_canvas_ms[avg=3.3 p95=6.6 max=7.9] evt_frame_ms[avg=9.8 p95=16.1 max=16.5] frames=19 render_ms[avg=1.8 p95=3.6 max=3.6] malen_us_je_punkt=292.9
LATENZ roh=31 angekommen=31 dichte=100.0% evt_app_ms[avg=2.4 p95=3.2 max=5.1] evt_canvas_ms[avg=2.9 p95=5.4 max=5.6] evt_frame_ms[avg=11.7 p95=30.4 max=36.7] frames=13 render_ms[avg=1.6 p95=2.6 max=2.6] malen_us_je_punkt=288.4
LATENZ roh=89 angekommen=89 dichte=100.0% evt_app_ms[avg=2.3 p95=3.0 max=5.0] evt_canvas_ms[avg=2.9 p95=4.1 max=5.5] evt_frame_ms[avg=9.3 p95=14.1 max=23.6] frames=43 render_ms[avg=1.5 p95=1.8 max=2.9] malen_us_je_punkt=283.8
LATENZ roh=31 angekommen=29 dichte=93.5% evt_app_ms[avg=3.1 p95=9.7 max=13.2] evt_canvas_ms[avg=3.2 p95=5.3 max=5.7] evt_frame_ms[avg=12.5 p95=33.9 max=40.4] frames=13 render_ms[avg=1.7 p95=3.1 max=3.1] malen_us_je_punkt=317.0
LATENZ roh=57 angekommen=57 dichte=100.0% evt_app_ms[avg=2.4 p95=3.4 max=4.0] evt_canvas_ms[avg=3.0 p95=4.8 max=5.1] evt_frame_ms[avg=10.7 p95=28.1 max=38.0] frames=26 render_ms[avg=1.7 p95=2.9 max=3.0] malen_us_je_punkt=315.1
LATENZ roh=56 angekommen=56 dichte=100.0% evt_app_ms[avg=2.4 p95=3.6 max=5.1] evt_canvas_ms[avg=3.0 p95=4.9 max=5.7] evt_frame_ms[avg=9.1 p95=13.3 max=15.0] frames=28 render_ms[avg=1.6 p95=2.5 max=2.7] malen_us_je_punkt=288.0
LATENZ roh=80 angekommen=73 dichte=91.3% evt_app_ms[avg=3.3 p95=11.2 max=18.3] evt_canvas_ms[avg=3.2 p95=6.6 max=11.8] evt_frame_ms[avg=10.1 p95=21.1 max=30.1] frames=38 render_ms[avg=1.6 p95=3.2 max=3.2] malen_us_je_punkt=291.5
LATENZ roh=2708 angekommen=2230 dichte=82.3% evt_app_ms[avg=2.3 p95=3.2 max=11.0] evt_canvas_ms[avg=2.8 p95=2.9 max=9.1] evt_frame_ms[avg=8.8 p95=10.7 max=18.7] frames=1115 render_ms[avg=2.2 p95=5.2 max=199.0] malen_us_je_punkt=243.3
LATENZ roh=331 angekommen=297 dichte=89.7% evt_app_ms[avg=2.3 p95=3.2 max=13.7] evt_canvas_ms[avg=2.8 p95=2.8 max=6.6] evt_frame_ms[avg=8.9 p95=10.8 max=23.7] frames=149 render_ms[avg=2.0 p95=1.9 max=46.2] malen_us_je_punkt=247.7
LATENZ roh=105 angekommen=85 dichte=81.0% evt_app_ms[avg=2.5 p95=3.2 max=9.3] evt_canvas_ms[avg=2.8 p95=3.0 max=5.5] evt_frame_ms[avg=9.0 p95=10.9 max=19.2] frames=43 render_ms[avg=2.8 p95=2.0 max=50.4] malen_us_je_punkt=255.6
LATENZ roh=692 angekommen=248 dichte=35.8% evt_app_ms[avg=3.0 p95=3.3 max=42.2] evt_canvas_ms[avg=2.8 p95=2.8 max=6.5] evt_frame_ms[avg=8.9 p95=10.8 max=17.2] frames=125 render_ms[avg=4.5 p95=29.1 max=59.2] malen_us_je_punkt=248.2
LATENZ roh=33 angekommen=33 dichte=100.0% evt_app_ms[avg=2.5 p95=4.7 max=5.3] evt_canvas_ms[avg=3.3 p95=6.4 max=7.0] evt_frame_ms[avg=9.9 p95=14.0 max=14.8] frames=16 render_ms[avg=2.0 p95=4.8 max=4.8] malen_us_je_punkt=287.3
LATENZ roh=34 angekommen=32 dichte=94.1% evt_app_ms[avg=3.4 p95=12.0 max=15.5] evt_canvas_ms[avg=3.7 p95=8.4 max=8.7] evt_frame_ms[avg=12.3 p95=24.5 max=28.1] frames=16 render_ms[avg=1.9 p95=2.5 max=2.5] malen_us_je_punkt=342.6
LATENZ roh=25 angekommen=25 dichte=100.0% evt_app_ms[avg=2.4 p95=2.9 max=4.8] evt_canvas_ms[avg=3.1 p95=5.0 max=6.5] evt_frame_ms[avg=9.5 p95=14.6 max=15.2] frames=13 render_ms[avg=1.9 p95=2.8 max=2.8] malen_us_je_punkt=294.9
LATENZ roh=33 angekommen=33 dichte=100.0% evt_app_ms[avg=2.5 p95=5.2 max=5.3] evt_canvas_ms[avg=3.1 p95=6.1 max=6.9] evt_frame_ms[avg=9.3 p95=15.0 max=15.1] frames=15 render_ms[avg=1.8 p95=2.5 max=2.5] malen_us_je_punkt=280.4
LATENZ roh=178 angekommen=77 dichte=43.3% evt_app_ms[avg=3.1 p95=3.5 max=23.5] evt_canvas_ms[avg=3.0 p95=4.7 max=5.9] evt_frame_ms[avg=9.5 p95=13.7 max=28.8] frames=41 render_ms[avg=1.7 p95=2.0 max=3.3] malen_us_je_punkt=262.5
LATENZ roh=70 angekommen=70 dichte=100.0% evt_app_ms[avg=2.3 p95=3.0 max=4.8] evt_canvas_ms[avg=3.0 p95=4.5 max=6.5] evt_frame_ms[avg=9.4 p95=14.6 max=20.5] frames=34 render_ms[avg=1.7 p95=2.9 max=3.0] malen_us_je_punkt=270.3
LATENZ roh=189 angekommen=189 dichte=100.0% evt_app_ms[avg=2.3 p95=2.8 max=4.5] evt_canvas_ms[avg=2.8 p95=3.8 max=5.7] evt_frame_ms[avg=9.4 p95=13.0 max=32.6] frames=93 render_ms[avg=1.7 p95=2.4 max=3.7] malen_us_je_punkt=260.1
LATENZ roh=229 angekommen=87 dichte=38.0% evt_app_ms[avg=2.6 p95=3.5 max=6.2] evt_canvas_ms[avg=3.0 p95=4.8 max=7.5] evt_frame_ms[avg=9.5 p95=12.9 max=20.6] frames=44 render_ms[avg=1.8 p95=2.5 max=3.0] malen_us_je_punkt=279.3
LATENZ roh=1 angekommen=1 dichte=100.0% evt_app_ms[avg=3.2 p95=3.2 max=3.2] evt_canvas_ms[avg=5.7 p95=5.7 max=5.7] evt_frame_ms[avg=13.7 p95=13.7 max=13.7] frames=1 render_ms[avg=2.8 p95=2.8 max=2.8] malen_us_je_punkt=288.7
LATENZ roh=535 angekommen=5 dichte=0.9% evt_app_ms[avg=3.1 p95=3.4 max=40.2] evt_canvas_ms[avg=2.9 p95=3.4 max=3.4] evt_frame_ms[avg=10.7 p95=15.5 max=15.5] frames=3 render_ms[avg=2.1 p95=2.7 max=2.7] malen_us_je_punkt=281.2
LATENZ roh=290 angekommen=2 dichte=0.7% evt_app_ms[avg=2.7 p95=4.4 max=9.9] evt_canvas_ms[avg=5.0 p95=5.7 max=5.7] evt_frame_ms[avg=12.7 p95=12.7 max=12.7] frames=1 render_ms[avg=4.2 p95=4.2 max=4.2] malen_us_je_punkt=330.5
LATENZ roh=736 angekommen=239 dichte=32.5% evt_app_ms[avg=2.9 p95=4.4 max=40.8] evt_canvas_ms[avg=3.0 p95=4.5 max=12.2] evt_frame_ms[avg=11.8 p95=33.3 max=57.7] frames=107 render_ms[avg=3.5 p95=7.8 max=44.9] malen_us_je_punkt=253.8
LATENZ roh=203 angekommen=5 dichte=2.5% evt_app_ms[avg=3.7 p95=3.3 max=35.7] evt_canvas_ms[avg=4.5 p95=6.3 max=6.3] evt_frame_ms[avg=160.6 p95=310.0 max=310.0] frames=2 render_ms[avg=41.9 p95=49.9 max=49.9] malen_us_je_punkt=643.5
LATENZ roh=281 angekommen=2 dichte=0.7% evt_app_ms[avg=3.1 p95=3.3 max=6.4] evt_canvas_ms[avg=3.2 p95=3.5 max=3.5] evt_frame_ms[avg=15.3 p95=17.1 max=17.1] frames=1 render_ms[avg=3.0 p95=3.0 max=3.0] malen_us_je_punkt=367.6
LATENZ roh=352 angekommen=12 dichte=3.4% evt_app_ms[avg=2.7 p95=5.0 max=7.6] evt_canvas_ms[avg=3.1 p95=4.2 max=4.2] evt_frame_ms[avg=11.3 p95=20.2 max=20.2] frames=6 render_ms[avg=12.3 p95=48.7 max=48.7] malen_us_je_punkt=325.6
LATENZ roh=118 angekommen=4 dichte=3.4% evt_app_ms[avg=2.8 p95=4.6 max=9.3] evt_canvas_ms[avg=4.5 p95=5.6 max=5.6] evt_frame_ms[avg=12.8 p95=18.9 max=18.9] frames=3 render_ms[avg=2.6 p95=3.3 max=3.3] malen_us_je_punkt=416.5
LATENZ roh=924 angekommen=782 dichte=84.6% evt_app_ms[avg=2.4 p95=3.3 max=15.0] evt_canvas_ms[avg=2.8 p95=2.9 max=7.8] evt_frame_ms[avg=9.0 p95=10.8 max=37.2] frames=388 render_ms[avg=1.6 p95=1.9 max=3.2] malen_us_je_punkt=250.7
LATENZ roh=687 angekommen=359 dichte=52.3% evt_app_ms[avg=3.6 p95=3.7 max=83.5] evt_canvas_ms[avg=3.2 p95=4.4 max=86.5] evt_frame_ms[avg=10.4 p95=16.1 max=95.9] frames=177 render_ms[avg=5.8 p95=5.6 max=195.6] malen_us_je_punkt=280.8
LATENZ roh=761 angekommen=240 dichte=31.5% evt_app_ms[avg=3.6 p95=3.3 max=80.2] evt_canvas_ms[avg=3.0 p95=4.6 max=6.8] evt_frame_ms[avg=10.2 p95=14.0 max=93.3] frames=122 render_ms[avg=5.2 p95=15.7 max=93.7] malen_us_je_punkt=280.5
LATENZ roh=535 angekommen=443 dichte=82.8% evt_app_ms[avg=2.4 p95=3.2 max=10.6] evt_canvas_ms[avg=2.8 p95=3.0 max=6.7] evt_frame_ms[avg=10.7 p95=14.2 max=94.0] frames=211 render_ms[avg=4.9 p95=5.5 max=174.7] malen_us_je_punkt=255.7
LATENZ roh=460 angekommen=456 dichte=99.1% evt_app_ms[avg=2.4 p95=3.3 max=21.1] evt_canvas_ms[avg=2.9 p95=3.7 max=8.6] evt_frame_ms[avg=9.3 p95=13.5 max=35.6] frames=223 render_ms[avg=1.6 p95=1.8 max=3.5] malen_us_je_punkt=260.5
LATENZ roh=199 angekommen=199 dichte=100.0% evt_app_ms[avg=2.5 p95=3.8 max=4.4] evt_canvas_ms[avg=3.1 p95=4.7 max=7.7] evt_frame_ms[avg=33.2 p95=83.9 max=1797.1] frames=61 render_ms[avg=1.8 p95=3.2 max=3.6] malen_us_je_punkt=268.7
LATENZ roh=102 angekommen=96 dichte=94.1% evt_app_ms[avg=3.3 p95=7.2 max=28.4] evt_canvas_ms[avg=3.0 p95=4.7 max=7.9] evt_frame_ms[avg=11.4 p95=30.5 max=50.5] frames=45 render_ms[avg=1.6 p95=2.0 max=2.1] malen_us_je_punkt=260.2
LATENZ roh=111 angekommen=105 dichte=94.6% evt_app_ms[avg=3.6 p95=11.6 max=35.0] evt_canvas_ms[avg=3.0 p95=4.7 max=7.6] evt_frame_ms[avg=9.5 p95=13.6 max=14.7] frames=53 render_ms[avg=1.7 p95=2.0 max=2.1] malen_us_je_punkt=261.8
LATENZ roh=123 angekommen=116 dichte=94.3% evt_app_ms[avg=3.6 p95=7.4 max=37.3] evt_canvas_ms[avg=2.9 p95=4.8 max=7.6] evt_frame_ms[avg=9.1 p95=12.7 max=15.4] frames=60 render_ms[avg=1.6 p95=2.2 max=2.7] malen_us_je_punkt=259.8
LATENZ roh=180 angekommen=176 dichte=97.8% evt_app_ms[avg=2.5 p95=3.3 max=6.6] evt_canvas_ms[avg=3.0 p95=4.5 max=11.7] evt_frame_ms[avg=36.3 p95=186.2 max=226.2] frames=64 render_ms[avg=1.7 p95=2.1 max=3.6] malen_us_je_punkt=263.9
LATENZ roh=481 angekommen=383 dichte=79.6% evt_app_ms[avg=3.7 p95=13.2 max=47.7] evt_canvas_ms[avg=2.8 p95=3.2 max=6.4] evt_frame_ms[avg=9.3 p95=12.9 max=29.9] frames=187 render_ms[avg=3.2 p95=3.5 max=47.0] malen_us_je_punkt=253.3
LATENZ roh=571 angekommen=571 dichte=100.0% evt_app_ms[avg=2.2 p95=2.3 max=4.8] evt_canvas_ms[avg=2.8 p95=2.9 max=7.5] evt_frame_ms[avg=9.0 p95=10.9 max=18.2] frames=286 render_ms[avg=1.7 p95=1.9 max=3.8] malen_us_je_punkt=251.0
LATENZ roh=106 angekommen=106 dichte=100.0% evt_app_ms[avg=2.4 p95=4.1 max=5.0] evt_canvas_ms[avg=3.1 p95=5.3 max=7.4] evt_frame_ms[avg=10.1 p95=16.7 max=34.8] frames=52 render_ms[avg=1.8 p95=3.4 max=3.7] malen_us_je_punkt=285.9
LATENZ roh=250 angekommen=249 dichte=99.6% evt_app_ms[avg=2.3 p95=2.9 max=6.1] evt_canvas_ms[avg=2.9 p95=3.7 max=8.2] evt_frame_ms[avg=9.5 p95=13.0 max=35.9] frames=123 render_ms[avg=1.7 p95=2.0 max=3.8] malen_us_je_punkt=256.3
LATENZ roh=89 angekommen=82 dichte=92.1% evt_app_ms[avg=4.0 p95=17.2 max=37.0] evt_canvas_ms[avg=3.1 p95=5.4 max=7.8] evt_frame_ms[avg=9.4 p95=12.8 max=16.5] frames=41 render_ms[avg=1.7 p95=1.9 max=3.6] malen_us_je_punkt=255.9
LATENZ roh=566 angekommen=563 dichte=99.5% evt_app_ms[avg=2.4 p95=2.3 max=46.3] evt_canvas_ms[avg=2.9 p95=2.9 max=47.1] evt_frame_ms[avg=9.2 p95=10.9 max=56.1] frames=281 render_ms[avg=2.6 p95=2.0 max=47.3] malen_us_je_punkt=251.6
```
