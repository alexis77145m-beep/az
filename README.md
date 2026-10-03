# Metal Flanger
Flanger métal (feedback saturé) avec auto-pan lent gauche ↔ droite. VST3 + Standalone (JUCE).

Paramètres : Flanger Rate, Depth, Feedback (négatif = plus métallique), Metal Drive, Mix, Pan Rate, Pan Depth.

## Obtenir le plugin (sans rien installer)
1. Poussez ce dépôt sur GitHub, onglet **Actions → Build plugin** (gratuit).
2. Téléchargez l'artefact `MetalFlanger-windows-latest` (dossier `.vst3`).
3. Copiez-le dans `C:\Program Files\Common Files\VST3`, puis rescannez les plugins dans votre DAW.

## Build local
`cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`

Test du DSP : `g++ -std=c++17 test/test.cpp -o t && ./t`
