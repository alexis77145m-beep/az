# LSA Flanger
Flanger métal (feedback saturé) avec auto-pan lent gauche ↔ droite. VST3 + Standalone (JUCE).

Un seul bouton **Effect** : à 0 % le signal est strictement inchangé, en montant on ajoute le flanger métal et le balancement gauche-droite (niveau sonore compensé).

## Obtenir le plugin (sans rien installer)
1. Poussez ce dépôt sur GitHub, onglet **Actions → Build plugin** (gratuit).
2. Téléchargez l'artefact `LSAFlanger-windows-latest` (dossier `.vst3`).
3. Copiez-le dans `C:\Program Files\Common Files\VST3`, puis rescannez les plugins dans votre DAW.

## Build local
`cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`

Tests DSP : `g++ -std=c++17 test/macro_test.cpp -o m && ./m` (niveau, stabilité, bypass exact à 0)
