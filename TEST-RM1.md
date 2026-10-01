# Gerätetest reMarkable 1

Anleitung für den Test von **Hello-World** und **reMarkable SP** auf einem reMarkable 1.

- **Ablauf:** Jeder Schritt wird einzeln vom Besitzer des Geräts freigegeben. Erst nach der
  Freigabe ausführen, danach das Ergebnis mit der Erwartung vergleichen und berichten.
- **Abweichungen:** Weicht ein Ergebnis ab, **anhalten** und berichten. Nicht improvisieren.

**Gerät laut Besitzer:** reMarkable 1, Firmware 3.26.0.68, armv7l, mit xovi, AppLoad und literm.
Das Root-Dateisystem ist fast voll.

**Grundregeln**
- Nichts nach `/usr`, `/lib`, `/etc` oder `/opt` schreiben. Kein Autostart und keine
  systemd-Dienste anlegen.
- Alles liegt unter `/home/root/apps/<app>`.
- xochitl wird nur in den dafür vorgesehenen Schritten gestoppt und gestartet.
- SSH per USB: `ssh root@10.11.99.1`. Befehle mit `PC$` laufen am Rechner im Ordner dieses
  Repos, Befehle mit `rM#` laufen auf dem Gerät.
- **Notfall:** Bleibt der Bildschirm hängen, `rM# systemctl start xochitl` ausführen. Hilft
  das nicht, die Ein-/Aus-Taste 10 s gedrückt halten (Neustart). Die Apps starten nie
  automatisch.

**Gebaut mit:** dem offiziellen SDK `remarkable-production-image-5.6.75-rm1-public-x86_64-toolchain.sh`
für reMarkable OS 3.26.0.68 (Qt 6.8.2), Quelle <https://developer.remarkable.com/links>.

---

## 0. Dateien holen (PC)

Release `v0.3.1` von <https://github.com/motus5/remarkable-sp/releases>:
`hello-remarkable-rm1.tar.gz`, `remarkable-sp-rm1-0.3.1.tar.gz`, `SHA256SUMS`.

```sh
PC$ git clone -b feature/remarkable-sp-app https://github.com/motus5/remarkable-sp && cd remarkable-sp
PC$ # Release-Dateien in diesen Ordner legen, dann:
PC$ sha256sum -c --ignore-missing SHA256SUMS
```
**Erwartet:** `hello-remarkable-rm1.tar.gz: OK` und `remarkable-sp-rm1-0.3.1.tar.gz: OK`.

## 1. Vorab-Prüfungen (nur lesen)

```sh
rM# cat /etc/os-release; uname -a; df -h / /home
rM# grep -i version /usr/share/remarkable/update.conf 2>/dev/null
rM# ls -l /usr/lib/libQt6Core.so.6 /usr/lib/libQt6Gui.so.6 /usr/lib/libQt6Qml.so.6 \
          /usr/lib/libQt6Quick.so.6 /usr/lib/libQt6Network.so.6 /usr/lib/libcrypto.so.3 /usr/lib/libz.so.1
rM# ls /usr/lib/plugins/platforms/ /usr/lib/plugins/scenegraph/
rM# grep -E '^N: Name|^H: Handlers' /proc/bus/input/devices
rM# systemctl is-active xochitl
rM# ls -la /home/root/apps 2>/dev/null || echo "kein apps-Ordner"
```

| Prüfung | Erwartet | Wenn nicht |
|---|---|---|
| `os-release` | `IMG_VERSION="5.6.75"` (SDK-Stand zu 3.26.0.68); eine andere Ausgabe ist möglich, bitte notieren | weiter, Version berichten |
| `uname -a` | `armv7l` | **anhalten** |
| `df -h /home` | mindestens **10 MB** frei | **anhalten** |
| Qt-6-Bibliotheken | alle 7 Dateien vorhanden | **anhalten**, fehlende nennen |
| `plugins/scenegraph` | enthält `libqsgepaper.so` | **anhalten** |
| `plugins/platforms` | enthält `libepaper.so`; fehlt sie, nutzen die Apps automatisch die mitgelieferte | weiter, Ergebnis notieren |
| Eingabegeräte | ein Wacom-Digitizer (Stift), ein Touchscreen, Tasten; Namen und `eventN` notieren | weiter, Liste berichten |
| xochitl | `active` | notieren |

## 2. Hello-World installieren (PC)

```sh
PC$ deploy/install.sh hello-remarkable-rm1.tar.gz
```
**Erwartet:**
- Die Ausgabe zeigt `Installiert: 0.3.1 in /home/root/apps/hello-remarkable`.
- Die Liste enthält `hello_remarkable`, `start.sh`, `fallback/`, `licenses/` und `VERSION`.
- Danach `rM# df -h /home`: etwa 0,3 MB weniger frei als vorher.

## 3. xochitl stoppen (eigener Schritt)

```sh
rM# systemctl stop xochitl; systemctl is-active xochitl
```
**Erwartet:** `inactive`. Der Bildschirm zeigt weiter das letzte Bild, das ist normal.

## 4. Hello-World starten und beenden

```sh
rM# /home/root/apps/hello-remarkable/start.sh
```
**Erwartet:**
- Der Bildschirm wird weiß, mit dickem schwarzem Rahmen, dem Text **„Hello reMarkable 1“**,
  einer Zeile mit der Bildschirmgröße (1404 × 1872) und einem schwarzen Quadrat.
- **Jedes Tippen mit dem Finger** blendet das Quadrat aus bzw. ein.
- Steht im Terminal `start: using bundled epaper platform plugin`, wird das mitgelieferte
  Plugin benutzt (siehe Schritt 1).

Beenden mit **Strg+C**. Berichten: Bild ja/nein, Text lesbar ja/nein, Tippen wirkt ja/nein und
die komplette Terminal-Ausgabe. Kein Bild oder kein Text: anhalten, Ausgabe berichten.

## 5. xochitl starten (eigener Schritt)

```sh
rM# systemctl start xochitl; systemctl is-active xochitl
```
**Erwartet:** `active`, die normale reMarkable-Oberfläche erscheint.

## 6. App installieren (PC)

```sh
PC$ deploy/install.sh remarkable-sp-rm1-0.3.1.tar.gz
```
**Erwartet:** `Installiert: 0.3.1 in /home/root/apps/remarkable-sp`, rund 2,7 MB belegt.

## 7. xochitl stoppen, App starten

```sh
rM# systemctl stop xochitl
rM# /home/root/apps/remarkable-sp/start.sh
```
**Erwartet im Terminal:** eine Zeile
`Pen digitizer /dev/input/eventN (x 0..…, y 0..…, pressure 0..…), transform swapxy,invy`.
**Erwartet auf dem Bildschirm:** links die Icon-Leiste, oben „Heute“, rechts ein runder ＋-Knopf.

Prüfen und je Punkt ja/nein berichten:

| # | Aktion | Erwartet |
|---|---|---|
| a | ＋ mit dem Finger antippen | Notizbuchseite öffnet sich, links die Werkzeugleiste |
| b | Mit dem Stift oben links ins Titelfeld schreiben | Tinte erscheint **direkt unter der Stiftspitze** |
| c | Mit dem Radierer-Ende über die Tinte | Tinte verschwindet |
| d | Handballen auf das Papier legen | keine Striche |
| e | ⌨ antippen | Bildschirmtastatur erscheint, Tippen schreibt ins Feld |
| f | „Schließen“ unten links | zurück zur Liste, Aufgabe mit Handschrift-Titel sichtbar |
| g | Aufgabe lange drücken | Kontextmenü erscheint |

**Stift falsch (b):**
1. App mit Strg+C beenden und notieren, wo die Tinte statt unter der Stiftspitze erscheint
   (gespiegelt, gedreht).
2. Die Belegungen der Reihe nach probieren, bis die Tinte unter der Spitze erscheint:
   ```sh
   rM# RMSP_PEN_TRANSFORM="swapxy,invx" /home/root/apps/remarkable-sp/start.sh
   ```
   Werte: `swapxy,invx` · `swapxy` · `swapxy,invx,invy` · `invy` · `invx` · `invx,invy` · `none`
3. Den passenden Wert berichten.

**Touch falsch (a, Tipp landet woanders):**
`QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="rotate=0"` vor den Start setzen, ebenso `rotate=90` und
`rotate=270`, und berichten, welcher Wert stimmt.

Beenden mit **Strg+C**, danach:
```sh
rM# ls -la /home/root/apps/remarkable-sp/data
```
**Erwartet:** `sp-state.json`, `sp-client.json`, `settings.ini` (falls etwas eingestellt wurde) und `ink/`.

## 8. xochitl starten (eigener Schritt)

```sh
rM# systemctl start xochitl; systemctl is-active xochitl
```
**Erwartet:** `active`, die normale Oberfläche ist zurück.

## 9. Rückbau (PC)

```sh
PC$ deploy/uninstall.sh hello-remarkable
PC$ deploy/uninstall.sh remarkable-sp              # behält data/
PC$ deploy/uninstall.sh remarkable-sp root@10.11.99.1 --purge   # löscht alles
rM# ls -la /home/root/apps 2>/dev/null || echo "apps-Ordner entfernt"; df -h /home
```
**Erwartet:**
- `uninstall` meldet „Entfernt: …“ bzw. „Programm entfernt, Daten behalten“.
- Nach `--purge` ist `/home/root/apps` weg, sofern dort nichts anderes lag.
- Der freie Platz entspricht dem Wert aus Schritt 1.

## Bericht

Bitte pro Schritt: Befehl, vollständige Ausgabe, Beobachtung am Bildschirm, ja/nein zur
Erwartung. Besonders wichtig sind:
- die Ausgaben aus Schritt 1,
- die Terminal-Ausgaben aus Schritt 4 und 7,
- falls nötig der passende `RMSP_PEN_TRANSFORM`- bzw. Touch-Wert.
