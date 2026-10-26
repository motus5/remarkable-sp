# Auftrag: Bilder als PDF aufs reMarkable 1 (z. B. Tafelfotos, Arbeitsblätter)

Für eine Entwickler-Instanz mit GitHub-Zugriff (Repo `motus5/remarkable-sp`). Vorher
`docs/auftraege/LEARNINGS-RM1.md` vollständig lesen – das ist verbindlich.

## Ziel
Der Besitzer fotografiert in der Berufsschule (Handy) Tafelbilder oder Arbeitsblätter und will sie
auf dem rM1 als PDF öffnen und darauf schreiben. Mehrere Bilder → ein mehrseitiges PDF.

## Wichtig vorab
Das rM1 hat **keine Kamera** und nur ≈ 4 MB frei auf `/`. Die Umwandlung gehört deshalb
**nicht aufs Gerät**, sondern auf den PC im Heimnetz (Linux, dort läuft auch der eigene Sync
**rmfakecloud**). Aufs Tablet kommt nur das fertige PDF.

## Phase 1 – Recherche (zuerst, kein Code)
Kurzbericht `docs/auftraege/RECHERCHE-BILD-ZU-PDF.md` als PR, **bevor** du baust:
- Umwandlung: `img2pdf` (verlustfrei, LGPLv3), ImageMagick, OCRmyPDF (Texterkennung, optional), Werkzeuge zum Entzerren/Kontrast für Dokumentfotos (z. B. „scan“-artige Filter).
- Übertragung aufs Tablet: Upload an **rmfakecloud** (Weboberfläche oder Schnittstelle; Werkzeuge wie `rmapi` prüfen, die rmfakecloud als Ziel unterstützen), alternativ Handy-App/Weg, den es schon gibt.
- Eingang: Wie kommt das Foto vom Handy auf den PC – geteilter Ordner, Nextcloud, E-Mail, Upload-Seite? Vorhandenes nutzen.
- Je Baustein: Lizenz, Pflege, Aufwand. Empfehlung mit Begründung.

**Stopp nach Phase 1.** Der Besitzer gibt frei.

## Phase 2 – Bauen (nach Freigabe)
- Ein Befehl bzw. Eingangsordner: Bilder rein → PDF raus → im rM1 sichtbar.
- Für E-Ink aufbereiten: Graustufen, Kontrast, Rand beschneiden, gerade ausrichten (wenn ohne großen Aufwand).
- **Seitengröße 157,8 × 210,4 mm** (Bildfläche des rM1), damit nichts verkleinert wird; Hochformat.
- Dateiname mit Datum und Fach (z. B. `2026-10-27 Mathe Tafel.pdf`), Ablage in einem festen Ordner auf dem Tablet.
- Tests in CI für die Umwandlung (Seitenzahl, Seitengröße, Graustufen).

## Regeln für rmfakecloud (aus Erfahrung)
- Daten von rmfakecloud **nur lesen oder neu hochladen, nie löschen oder umbenennen** – Löschen dort löscht auf dem Tablet.
- Den rmfakecloud-Dienst nicht umkonfigurieren oder neu starten ohne Freigabe.

## Nicht erlaubt
Nichts auf dem rM1 installieren (nicht nötig), keine Systemdateien, kein Passwort/keine Adressen im Repo.
