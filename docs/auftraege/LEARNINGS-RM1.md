# Learnings für Arbeit am reMarkable 1 (Stand 10/2026)

Gilt für jede Instanz, die für das rM1 des Besitzers entwickelt oder testet. Belegt im
Gerätetest v0.3.1 (`docs/geraetetest/2026-10-rm1-v0.3.1/`). Kein Passwort, keine Adressen hier.

## Gerät
- reMarkable 1, OS 3.26.0.68 (`VERSION_ID=5.6.75`), Kernel 5.4.70, armv7l, Qt 6.8.2 im System.
- Bildschirm 1404 × 1872 px, 226 ppi (Bildfläche ≈ 157,8 × 210,4 mm). Stift event0 (0..20967 × 0..15725), Touch event2.
- Shell ist **BusyBox**: `head -n 5`, kein `ps aux`, kein GNU-`find`-Komfort.
- **`/` ist zu 98 % voll** (≈ 4 MB frei) und schrumpft durch das Systemjournal. Nie etwas nach `/usr`, `/etc`, `/opt` schreiben. Platz gibt es nur auf `/home` (≈ 6 GB).
- Installiert: **xovi** mit **AppLoad** (eigene Apps erscheinen im AppLoad-Menü), literm, touch-lock, KOReader u. a. Eigener Sync über **rmfakecloud** auf einem PC im Heimnetz.
- Die normale Oberfläche **xochitl** setzt bei jedem Start `/etc/passwd` und `/etc/shadow` neu → selbst gesetzte Root-Passwörter halten nicht.

## Die teuerste Lektion
- **`systemctl stop xochitl` macht das Tablet unbedienbar** (letztes Bild steht, nichts reagiert). Für den Besitzer sieht das wie ein Absturz aus – im Test zweimal passiert.
- Darum für neue Apps zuerst prüfen: **Geht es als AppLoad-App innerhalb von xochitl?** Dann muss xochitl nie gestoppt werden. Eigenständige Vollbild-Apps (wie reMarkable SP) nur, wenn es wirklich nötig ist.

## Bekannte Fallen bei Qt/QML auf dem rM1
- Fenster **nicht** fest dimensionieren: `width: Screen.width; height: Screen.height`. `visibility: FullScreen` allein greift unter epaper nicht (v0.3.1 belegte nur ein Viertel).
- E-Ink ist langsam: Teilauffrischung DU ≈ 320–410 ms, GC16 ≈ 810 ms (gemessen). Keine Animationen, kein Antialiasing für Linien, große Trefferflächen, Schwarz/Weiß bevorzugen.
- Archive mit `tar --owner=0 --group=0 --numeric-owner` packen.

## Zusammenarbeit
- **GitHub ist der gemeinsame Kanal**: Aufträge unter `docs/auftraege/`, Befunde als Issues, Testberichte als PR.
- Entwickler-Instanz baut und veröffentlicht Releases; die **Test-Instanz** am PC des Besitzers testet am Gerät (nur mit ihm am Gerät, jeder Geräteschritt mit seinem „ok“).
- Erst **recherchieren und vorhandene Lösungen prüfen** (Lizenz, Pflege, Eignung fürs rM1), dann nur das Nötige selbst bauen. Recherche-Ergebnis vor dem Bauen als Kurzbericht vorlegen.
- Öffentliches Repo: nie Passwörter, Heimnetz-Adressen, Rechner- oder Personennamen.
- Ordner- und Projektnamen ausschreiben, keine Kürzel.
