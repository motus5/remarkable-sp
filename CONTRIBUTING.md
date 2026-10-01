# Mitmachen

Danke für dein Interesse! Beiträge aller Art sind willkommen: Fehlerberichte, Tests auf echter
Hardware, Übersetzungen, Code.

## Ablauf

1. Issue anlegen oder ein bestehendes kommentieren. Bei größeren Änderungen vorher kurz abstimmen.
2. Fork und Branch anlegen, dann ändern.
3. Lokal prüfen:
   ```sh
   cmake -S . -B build && cmake --build build -j
   QT_QPA_PLATFORM=offscreen ./build/rmsp_tests
   ```
4. Pull Request mit kurzer Beschreibung und, bei UI-Änderungen, einem Screenshot.

## Leitlinien

- **E-Ink zuerst:** keine Animationen, Schwarz/Weiß plus `Theme.rule`-Grau, große Tippflächen
  (≥ 7u) und seitenweises Blättern statt Scrollen.
- **Stift schreibt, Finger bedient:** Neue Eingabeflächen müssen das beachten
  (`PointHandler` mit `acceptedDevices`).
- **Sync-Kompatibilität:**
  - Änderungen am Datenmodell laufen immer über Operationen in `SpStore`, im Format von
    Super Productivity.
  - Unbekannte Felder müssen erhalten bleiben.
  - Neue Operationstypen bitte gegen den SP-Quellcode prüfen: `src/app/op-log/`,
    `root-store/meta/task-shared-meta-reducers/`.
- **Stil:** Code wie der umgebende, kurze Kommentare nur, wo das „Warum“ nicht offensichtlich ist.
- **Tests:** Neue Logik in `tests/tst_core.cpp` abdecken, besonders alles rund um den Sync.

## Auf dem Gerät testen

Rückmeldungen von echter Hardware (rM1, rM2, Paper Pro) helfen am meisten. Bitte angeben:
Gerät, Firmware-Version, Launcher und was genau passiert ist.
