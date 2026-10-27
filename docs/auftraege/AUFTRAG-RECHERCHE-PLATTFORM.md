# Auftrag: Recherche Plattform, Start ohne PC, Stift und Programmiersprache

Für eine Instanz mit Lese- und Schreibzugriff auf GitHub (Repo `motus5/remarkable-sp`). Das ist
**nur Recherche**: kein Code im App-Branch, kein Zugriff auf das Gerät.

## Vorher lesen (verbindlich)

1. `docs/LEARNINGS.md`: Gerät, bekannte Fallen, Regeln der Zusammenarbeit.
2. `docs/PRUEFUNG-START-OHNE-PC.md`: Stand zu AppLoad und die Optionen A/B/C.
3. `docs/geraetetest/2026-10-rm1-v0.3.2/BERICHT-RM1-v0.3.2.md`: letzte Messwerte.
4. `README.md` und `src/peninput.cpp`, `src/palmfilter.cpp`, `src/inkcanvas.cpp`: der heutige
   Stiftweg.

## Warum

Viele Fehler der letzten Versionen kamen aus Plattform-Wissen, das erst am Gerät auffiel:

- Vollbild unter epaper,
- fehlende Schriftzeichen,
- Touch bricht Stiftstriche ab,
- xochitl-Stopp wirkt wie ein Absturz.

Die Recherche soll solche Fallen **vor** dem Bauen sichtbar machen und eine belegte Grundlage für
die nächsten Entscheidungen liefern.

## Fragen

**F1 – Start ohne PC (Issue #15).** Wie startet man eine Qt-6-App (QML plus eigene C++-Typen)
am rM1 mit OS 3.26 direkt am Gerät und kommt zuverlässig zu xochitl zurück?

- AppLoad, externe App mit `qtfb`:
  - Gibt es dafür ein QPA-Plugin oder einen Shim für Qt 6?
  - Läuft das am rM1 (32 Bit, armv7)?
  - Wie kommen Stift (mit Druck) und Touch an?
  - Welche Latenz hat das?
- Wie machen es vorhandene Apps am rM1, z. B. KOReader, Paperterm, Riddle oder der AppLoad-Store?
  Mit Manifest-Beispielen.
- Übergabe-Startknopf (Option C):
  - Überlebt ein von xochitl gestartetes Skript `systemctl stop xochitl`?
  - Ist `systemd-run` auf OS 3.26 vorhanden?
  - Gibt es einen etablierten Launcher, der genau das tut, mit Notausgang?
- Was passiert mit einer laufenden App, wenn das Gerät schlafen geht (Einschaltknopf, Deckel)?

**F2 – Stift und E-Ink.** Wie erreichen xochitl und andere Apps am rM1 schnelle Tinte?

- Wellenformen bzw. Auffrischungsmodi (DU, A2, GC16): Was bietet das epaper-Backend von
  reMarkable für Qt 6 (`libqsgepaper`, `developer.remarkable.com/documentation/qt_epaper`) an
  Schnittstellen, um für Tinte einen schnellen Modus zu wählen?
- Direkt in den Bildspeicher zeichnen (wie rmkit oder libremarkable) gegenüber Qt Quick: Welche
  Latenzen sind belegt?
- Palm-Rejection: Wie lösen es xochitl und andere Apps (Stift-Nähe, Kontaktgröße, Zeitfenster)?
- Ist ein Radierer pixelgenau oder ganze Striche üblich, und wie wird er umgesetzt?

**F3 – Programmiersprache und Toolkit.** Ist C++/Qt 6/QML (heute) die richtige Wahl für die
nächsten 1–2 Jahre?

- Vergleich mit Rust und libremarkable, rmkit (C++) sowie einer reinen AppLoad-QML-App.
- **Kriterien:**
  - rM1 und rM2 (später evtl. Paper Pro) aus einer Codebasis,
  - Pflege und Aktivität der Projekte (letzter Commit, Releases),
  - Lizenz,
  - Stift-Latenz,
  - Weiterverwendung des vorhandenen Sync-Codes (Format von Super Productivity, WebDAV,
    Argon2id + AES-GCM),
  - Aufwand eines Umstiegs in Personentagen (grob),
  - Testbarkeit ohne Gerät.
- Ergebnis: **Empfehlung mit Begründung**, ausdrücklich auch „bleiben“.

**F4 – Fallen-Liste.** Bekannte Probleme beim Entwickeln für reMarkable OS 3.x, die in
`docs/LEARNINGS.md` noch fehlen, z. B.:

- Schlafmodus und Einschaltknopf,
- Geisterbilder und volle Auffrischung,
- Updates des Systems, die Apps brechen,
- Schriftarten, Tastatur, Rotation,
- Unterschiede zwischen rM1 und rM2.

Je Falle: Folge, Abhilfe, Quelle.

**F5 – Vorhandene Lösungen.** Gibt es Aufgaben-, Zeiterfassungs- oder Super-Productivity-nahe Apps
fürs reMarkable, deren Teile man übernehmen könnte (Lizenz beachten)?

## Regeln

- **Quellen:**
  - Jede Aussage mit Quelle: Link, Datum des Abrufs, bei Code mit Datei und Zeile oder Commit.
  - Trennen in **belegt** (Doku, Code, eigener Test ohne Gerät) und **unbelegt** (Forum, Vermutung).
  - Angaben anderer Quellen gegenprüfen, wo möglich, z. B. im Quellcode statt nur im README.
- **Ohne Gerät:** Nichts am Gerät ausführen. Befehle, die am Gerät nötig wären, als „nur lesen,
  mit dem Besitzer“ aufschreiben.
- **Branches:** Keine Änderungen an `feature/remarkable-sp-app` oder `main`. Kleine Experimente
  (z. B. ein qtfb-Hallo-Welt) nur unter `recherche/` und nur, wenn sie ohne Gerät prüfbar sind.
- **Commits:**
  - Autor `motus5 <47573206+motus5@users.noreply.github.com>`.
  - **Keine** Co-Authored-By-Zeilen und keine Hinweise auf Werkzeuge oder Assistenten, weder in
    Commits noch in PRs, Issues oder Dateien.
- **Datenschutz:** Keine Passwörter, Adressen oder Personennamen.
- **Sprache:** Deutsch, kurz und klar; Tabellen, wo sie helfen.

## Lieferung (Rückkanal)

1. **Bericht:** Branch `recherche/plattform-2026-10` von `feature/remarkable-sp-app`, darin
   `docs/recherche/RECHERCHE-PLATTFORM-2026-10.md`:
   - zuerst eine Zusammenfassung mit je einer Empfehlung zu F1–F3,
   - dann die Abschnitte F1–F5,
   - am Ende „Offene Fragen an den Besitzer“ und „Vorschläge für LEARNINGS.md“, wörtlich
     einfügbar.
2. **Pull Request** gegen `feature/remarkable-sp-app` mit dem Titel
   „Recherche: Plattform, Start ohne PC, Stift, Sprache“, nur Dokumentation.
3. **Kommentar im Issue „Recherche: Plattform, Start ohne PC, Stift, Sprache“** (das Issue, das auf
   diesen Auftrag verweist), höchstens 15 Zeilen:
   - Link zum PR,
   - die drei Empfehlungen,
   - die wichtigsten neuen Fallen,
   - was belegt ist und was nicht.
   Rückfragen während der Arbeit ebenfalls dort, nicht im PR.
4. **Abschluss:** Dem Besitzer melden: „Recherche fertig: PR <Link>“. Der Besitzer gibt das an die
   Entwickler-Instanz weiter; die liest PR und Issue-Kommentar und plant danach.

**Nicht** Teil des Auftrags: Code der App ändern, Releases bauen, Issues schließen.
