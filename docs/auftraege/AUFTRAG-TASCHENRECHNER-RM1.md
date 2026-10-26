# Auftrag: Taschenrechner für das reMarkable 1

Für eine Entwickler-Instanz mit GitHub-Zugriff (Repo `motus5/remarkable-sp`). Vorher
`docs/auftraege/LEARNINGS-RM1.md` vollständig lesen – das ist verbindlich.

## Ziel
Ein Taschenrechner, den der Besitzer in der Berufsschule am rM1 nutzt: neben seinen Notizen,
schnell erreichbar, mit Finger bedienbar, gut lesbar auf E-Ink.

## Phase 1 – Recherche (zuerst, kein Code)
Das Rad nicht neu erfinden. Prüfe vorhandene Lösungen und lege einen Kurzbericht vor
(`docs/auftraege/RECHERCHE-TASCHENRECHNER.md`, als PR), **bevor** du baust:
- **Läuft es als AppLoad-App innerhalb von xochitl?** Das ist der bevorzugte Weg (kein Stopp der Oberfläche, siehe Learnings). Quellen: AppLoad-Doku und Beispiele (`github.com/asivery/rm-appload`, Wiki), App-Format in `github.com/maximerivest/remagic` (`docs/APP-SPEC.md`).
- Vorhandene Rechner: `github.com/reHackable/Calculator`, `rpncalc` aus `rmkit-dev/rmkit`, die Qt-Beispiel-App „Calqlatr“ (Qt Quick, als QML-Grundlage), weitere aus der Suche.
- Je Kandidat: Lizenz, letzte Pflege, Sprache/Toolkit, läuft auf OS 3.26 / Qt 6.8 ja/nein, Aufwand zum Anpassen.
- Empfehlung: übernehmen / umschreiben / neu – mit Begründung.

**Stopp nach Phase 1.** Der Besitzer gibt frei.

## Phase 2 – Bauen (nach Freigabe)
Mindestumfang:
- Grundrechenarten, Klammern, Prozent, Vorzeichen, Dezimalkomma; Punkt-vor-Strich korrekt.
- Für die Berufsschule: Quadrat, Wurzel, Potenz, Kehrwert, Speicher (M+, M−, MR, MC), Rückgängig der letzten Eingabe.
- Verlauf der letzten Rechnungen, sichtbar.
- Große Tasten (≥ 12 mm), Schwarz/Weiß, keine Animationen; Ergebnis groß.
- Kopieren des Ergebnisses ist nicht nötig.
- Rechenkern als **reine Funktion mit Unit-Tests** (mind. 30 Fälle inkl. Punkt-vor-Strich, Klammern, Division durch 0, Rundung 0,1+0,2) – in CI.
- QML-Test: Fenster füllt den Bildschirm 1404 × 1872.

## Lieferung
- Release mit Archiv, gepackt mit `--owner=0 --group=0 --numeric-owner`, Zielordner unter `/home/root/xovi/exthome/appload/<name>/` (bei AppLoad) bzw. `/home/root/apps/<name>/`.
- Install- und Uninstall-Skript, die **nur** in diesen Ordner schreiben.
- Testanleitung `TEST-RM1-TASCHENRECHNER.md` im Stil von `TEST-RM1.md`, aber kurz: Vorab lesen, installieren, AppLoad neu laden, Prüfliste, Rückbau. Kein Stopp von xochitl, wenn AppLoad.
- Hinweis: Schreiben unter `/home/root/xovi/…` braucht die ausdrückliche Freigabe des Besitzers.

## Nicht erlaubt
Kein Schreiben außerhalb des App-Ordners, keine Systemdateien, kein Autostart-Dienst, keine
Änderungen an xovi selbst, kein Passwort/keine Adressen im Repo.
