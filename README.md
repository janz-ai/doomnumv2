# DOOM-NW — Doom pour NumWorks 🔫

Un clone de Doom avec moteur raycaster 3D pour les calculatrices NumWorks (N0110 / N0120).

## Fonctionnalités

- **Moteur raycaster** (algorithme DDA, comme Wolfenstein 3D / Doom)
- **8 ennemis** avec IA (patrouille → chasse → attaque)
- **Arme pixel art** — fusil à pompe animé avec flash de bouche
- **HUD complet** — barre de vie, visage animé, score, munitions
- **Monstres pixel art** avec test de profondeur (z-buffer)
- **Effets** — flash rouge quand blessé, recul d'arme, bob de marche

## Contrôles

| Touche | Action |
|--------|--------|
| ↑      | Avancer |
| ↓      | Reculer |
| ←      | Tourner à gauche |
| →      | Tourner à droite |
| OK     | Tirer |
| Back   | Quitter |

## Prérequis de compilation

### 1. Installer Node.js et nwlink

```bash
# Sur macOS (avec Homebrew)
brew install node
npm install -g nwlink

# Sur Linux (Ubuntu/Debian)
sudo apt install nodejs npm
npm install -g nwlink

# Sur Windows
# Télécharger Node.js depuis https://nodejs.org
npm install -g nwlink
```

### 2. Installer le SDK NumWorks (Epsilon)

```bash
nwlink install-sdk
```

> Cela va cloner automatiquement le dépôt Epsilon et installer
> la chaîne de compilation ARM (arm-none-eabi-gcc).

### 3. Installer la toolchain ARM

```bash
# macOS
brew install arm-none-eabi-gcc

# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# Windows
# Télécharger depuis : https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain
```

## Compilation

```bash
cd doom-numworks
make
```

Le fichier `doom-nw.nwa` sera créé dans `output/`.

## Installation sur la calculatrice

### Méthode 1 — nwlink (recommandée)

1. Connecte ta NumWorks en USB
2. Lance l'application **NumWorks** sur ordinateur, ou ouvre
   [workshop.numworks.com](https://workshop.numworks.com)
3. ```bash
   make run
   ```
   ou
   ```bash
   nwlink send output/doom-nw.nwa
   ```

### Méthode 2 — Interface web

1. Va sur [workshop.numworks.com](https://workshop.numworks.com)
2. Connecte ta calculatrice en USB
3. Glisse-dépose le fichier `doom-nw.nwa`

## Structure du projet

```
doom-numworks/
├── Makefile          — Système de build NumWorks
├── app.json          — Métadonnées de l'application
├── README.md         — Ce fichier
└── src/
    ├── doom.h        — Déclarations, constantes, carte, structures
    ├── main.cpp      — Point d'entrée et boucle principale
    ├── game.cpp      — Logique de jeu, IA ennemis, physique
    ├── raycaster.cpp — Moteur raycaster (algorithme DDA)
    └── renderer.cpp  — Sprites ennemis, arme pixel art, HUD
```

## Architecture technique

### Moteur raycaster

Utilise l'algorithme **DDA (Digital Differential Analysis)** :
- 1 rayon par colonne d'écran (320 rayons total)
- Z-buffer par colonne pour le tri des sprites
- Correction fish-eye avec distance perpendiculaire
- 2 types de murs (brique rouge, pierre marron) avec faces N/S et E/W différentes

### Rendu des ennemis

- Sprites billboard (face toujours vers le joueur)
- Tri par profondeur (bubble sort, tableau de 8 max)
- Test de visibilité colonne par colonne via z-buffer
- Pixel art procédural (dessiné via fillRect)
- Variation de luminosité selon la distance

### IA ennemis

Machine à états : **Idle → Chase → Attack → Dying**
- Portée de détection : 8 unités
- Test de ligne de vue (DDA dans l'espace monde)
- Collision avec les murs (wall sliding)

## Dépannage

**"EPSILON_SOURCE_DIR not set"**
```bash
export EPSILON_SOURCE_DIR=$(nwlink epsilon-source-dir)
make
```

**Erreur arm-none-eabi-gcc**
```bash
# Vérifier l'installation
arm-none-eabi-gcc --version
# Si absent, installer (voir Prérequis)
```

**L'app ne s'affiche pas sur la calculatrice**
- Vérifier que ta NumWorks tourne sous Epsilon ≥ 16
- Mettre à jour via [numworks.com/fr/calculatrice/mise-a-jour](https://www.numworks.com/fr/calculatrice/mise-a-jour/)

---

*Projet créé avec amour et quelques démons pixelisés. Kill count maximal !* 🔥
