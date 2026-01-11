Drum Sampler — Kurzanleitung (für Entwickler)

## Ziel
Dieses Dokument beschreibt, wie das Projekt gebaut, getestet und die Offset‑Funktionalität verwendet wird.

## Voraussetzungen
- Visual Studio 2022 (x64)
- JUCE (modulpfad korrekt in Projekt gesetzt)
- C++17-Unterstützung

## Build
1. Öffne die Lösung: `Builds/VisualStudio2022/Juce-Drum-Sampler_StandalonePlugin.sln`
2. Konfiguration: `x64` + `Debug` oder `Release`
3. Build: **Build Solution** (Ctrl+Shift+B)

## Schnelltest (funktionelle Prüfungen)
1. Sample laden: Datei per Drag&Drop auf Pad 1/2 (oder mittels UI)
2. Waveform prüfen: Thumbnail muss sichtbar sein
3. Start‑Linie ziehen:
   - Linie mit der Maus auf der Waveform verschieben
   - Die Position wird intern als `Processor.newPositionSec` gespeichert
4. Play drücken:
   - `playFile(note)` setzt `CustomSamplerSound::startOffset` (0..0.9999)
   - `mSampler.noteOn(...)` startet die Stimme → `CustomSamplerVoice::startNote()` setzt `currentSamplePos`
   - `renderNextBlock()` gibt ab `currentSamplePos` wieder

## Wichtige Hinweise
- Offset‑Berechnung:
Clamp auf 0.9999 verhindert Start hinter dem letzten Sample (stille).
- ADSR/Parameter:
- Parameter werden im `AudioProcessorValueTreeState` angelegt.
- `updateADSR(index)` weist Werte an `CustomSamplerSound` zu.
- Debugging:
- Logs mit `DBG()` erscheinen in Visual Studio → Output.
- Prüfpunkte: `playFile()`, `startNote()`, `renderNextBlock()`, `updateADSR()`.

## Testcheckliste
- [ ] Sample lädt korrekt
- [ ] Linie ist verschiebbar
- [ ] Wiedergabe beginnt an der gesetzten Position
- [ ] Keine Stille bei Rechtsrand (extremer Offset)
- [ ] ADSR wirkt auf Wiedergabe
- [ ] Zwei Pads unabhängig testbar

## Troubleshooting
- Keine Datei gefunden: prüfe `#include`-Pfade (z. B. `CustomSamplerVoice.h`)
- Buildfehler: Clean → Rebuild; überprüfe JUCE‑Moduleinstellungen
- Keine Tonwiedergabe: Output‑Fenster prüfen (DBG), Sample korrekt geladen?

## Weiteres
Änderungen an Parametern oder an der Offset‑Logik bitte in `PluginProcessor.cpp` und `CustomSamplerVoice.h` vornehmen, danach neu bauen und die obigen Tests erneut durchführen.