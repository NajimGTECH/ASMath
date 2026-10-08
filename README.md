# ASMath – Bibliothèque mathématique ASM / SIMD

Projet GTech 3 (spécialisation moteur) – premier module du moteur physique.
Auteur : **Najim BAKKALI**.

La bibliothèque mathématique de GTech 2 (`Vector3`, `Mat4`, …) est auditée, corrigée, puis trois traitements par lots sont optimisés en **SIMD explicite (intrinsics SSE/SSE2)** et comparés à leur **référence C++** :

1. produits scalaires entre deux tableaux de `Vector3<float>` ;
2. normalisation d'un tableau de `Vector3<float>` ;
3. transformation de points 3D par une même matrice affine `Mat4<float>` (w = 1, sans division perspective).

Le projet contient aussi une **fonction en assembleur x64 (MASM)**, une comparaison **AoS / SoA**, des **tests automatisés** et une **application console de benchmark**.

📄 **Rapport technique** : [`docs/REPORT.md`](docs/REPORT.md) – audit, organisation mémoire, protocole, mesures, analyse, limites.
📄 **Désassemblage commenté** : [`docs/DISASSEMBLY.md`](docs/DISASSEMBLY.md).
📊 **Résultats bruts** : [`Benchmark/results/`](Benchmark/results).

---

## Structure du dépôt

```
SFML_Jour2.sln                 solution Visual Studio (tous les projets)
MathsLib/                      bibliothèque statique (C++20)
  include/
    Vector3.h, Mat4.h          socle existant (templates) – conventions documentées dans Mat4.h
    Vector3Batch.h             produit scalaire / normalisation par lots, Vec3SoA, conversions AoS <-> SoA
    Mat4Batch.h                transformation de points par lots, Mat4 x Mat4 (extension)
    AsmFunctions.h             déclarations des fonctions assembleur (extern "C")
  src/
    Vector3Batch.cpp           versions math::ref (C++) et math::simd (SSE)
    Mat4Batch.cpp
    SseHelpers.h               transposition AoS <-> SoA dans les registres (interne)
    asm/Dot3.asm               produit scalaire écrit en assembleur x64 (MASM)
UnitTests/                     tests MSTest C++ (Test Explorer de Visual Studio)
  UnitTests.cpp                tests du socle (Vector2/3, Mat3/4, Quaternion) + régressions de l'audit
  BatchTests.cpp               tests des traitements par lots, de l'ASM et des cas limites
  TestHelpers.h                tolérances justifiées, données aléatoires, tableaux à page protégée
Benchmark/                     application console de mesure
  src/main.cpp                 menu interactif + ligne de commande
  src/Workloads.cpp            liste des traitements et de leurs versions
  src/Measure.cpp              échauffement, calibration, échantillons, médiane / quartiles
  src/SystemInfo.cpp           CPU, jeux d'instructions, compilateur et options
  results/                     résultats bruts de la campagne de mesure (CSV)
docs/                          rapport, désassemblage commenté (+ ancienne doc Doxygen html)
SFML_Project1/                 ancien projet GTech 2 (jeu Asteroids SFML), non concerné par ce module
```

## Compilation

Prérequis : **Visual Studio 2022 ou plus récent**, charge de travail *Développement Desktop en C++* (contient MSVC v143, MASM `ml64` et le framework de tests C++). Windows x64.

1. Ouvrir `SFML_Jour2.sln`.
2. Choisir la configuration **Release | x64** (les mesures en Debug n'ont aucun sens).
3. *Générer > Générer la solution*.

En ligne de commande (*Developer PowerShell for VS*) :

```bat
msbuild SFML_Jour2.sln -p:Configuration=Release -p:Platform=x64
```

Options de compilation utilisées pour MathsLib et Benchmark (Release) : `/O2` (auto-vectorisation autorisée), `/fp:precise`, jeu d'instructions x64 par défaut (**SSE2**, pas d'AVX, pas de FMA), pas de `/GL` (pour pouvoir désassembler les `.obj`), `/Qvec-report:2` sur MathsLib (rapport d'auto-vectorisation dans la sortie de compilation). Le fichier `Dot3.asm` est assemblé par `ml64` grâce à la personnalisation de build MASM du projet.

## Lancer les tests

- Visual Studio : *Test > Explorateur de tests > Exécuter tout* (102 tests).
- Ligne de commande :

  ```bat
  vstest.console.exe x64\Release\UnitTests.dll /Platform:x64
  ```

Les tests vérifient chaque version (référence, SSE, SoA, ASM) contre des **valeurs connues**, contre un **calcul en double avec une borne d'erreur justifiée**, et entre elles. Les cas limites couverts : lot vide, tailles 0 à 40 et 1000-1003, vecteurs nuls, NaN, traitement en place, identité, translation, échelle, rotation, composition, et **accès hors limites** (tableaux collés à une page mémoire interdite). Détails : rapport § 6.

## Lancer les benchmarks

Toujours en **Release x64, sans débogueur** (Ctrl+F5 dans Visual Studio). Exécutable : `x64\Release\Benchmark.exe`.

```bat
Benchmark.exe                                   menu interactif : traitement, version, taille du lot
Benchmark.exe --list                            liste des traitements et versions
Benchmark.exe --op dot --version simd-aos --sizes 1000
Benchmark.exe --op normalize --version ref-aos,simd-aos,simd-soa --sizes 10,100000
Benchmark.exe --suite --out results             campagne complète (≈ 15 s), résultats bruts en CSV
```

| Option | Rôle (défaut) |
|---|---|
| `--op` | `dot`, `normalize`, `transform`, `convert`, `mat4mul` |
| `--version` | une ou plusieurs versions séparées par des virgules, ou `all` (défaut). La référence est toujours mesurée (pour le rapport de vitesse) |
| `--sizes` | tailles de lots (défaut : 10, 1000, 100000, 4000000 ; Mat4 : 10, 1000, 100000) |
| `--seed` | graine des données aléatoires (42) |
| `--samples` | nombre d'échantillons chronométrés (31) |
| `--out` | dossier des résultats bruts (`summary.csv`, `samples.csv`, `run_info.txt`) |

Le programme affiche le CPU, les jeux d'instructions, le compilateur et ses options, la commande exacte pour relancer la mesure, puis pour chaque version : **médiane** (ns par élément), **quartiles**, **dispersion (IQR %)**, **accélération** (temps référence / temps version) et le **résultat de la vérification** par rapport à la référence.

Protocole (détails : rapport § 7) : données générées avant la mesure (graine fixe), échauffement de 50 ms, calibration pour que chaque échantillon dure ≥ 2 ms, 31 échantillons, aucune allocation ni affichage pendant la mesure, appel par pointeur de fonction `volatile` pour que le calcul ne puisse pas être supprimé.

## Conventions de l'API

| Sujet | Convention |
|---|---|
| Stockage des matrices | **row-major**, `m[ligne][colonne]`, 16 floats contigus |
| Vecteurs | **vecteurs lignes** : `p' = p * M` ; translation dans la ligne 3 |
| Composition | `A * B` applique **A puis B** ; objet : `Scale * Rotate * Translate` (= `Mat4::TRS`) |
| Points / directions | points w = 1 (`MultiplyPoint`, `MultiplyPointAffine`), directions w = 0 (`MultiplyVector`) |
| Transformation par lots | w = 1, **pas de division perspective**, colonne 3 ignorée (matrice affine) |
| Vecteur nul | `Normalized()` et `NormalizeBatch` renvoient **(0, 0, 0)** si la longueur n'est pas > 0 (nul, NaN, sous-dépassement) |
| Lots | `n` quelconque (0 accepté, pas besoin d'un multiple de 4) ; aucun accès hors de `[0, n)` |
| Alignement | **aucun requis** (chargements non alignés) ; un `Vector3<float>` fait 12 octets, aligné sur 4 |
| Aliasing | entrée et sortie peuvent être identiques (en place), pas de recouvrement partiel |
| Versions | `math::ref::X` (référence C++) et `math::simd::X` (SSE) ont la même signature ; `math::x64asm` pour l'assembleur |

Exemple :

```cpp
#include "Vector3Batch.h"
#include "Mat4Batch.h"

std::vector<math::Vec3f> points = /* ... */;
std::vector<math::Vec3f> result(points.size());

const math::Mat4f m = math::Mat4f::TRS(position, rotation, scale);   // scale, puis rotation, puis translation
math::simd::TransformPointsBatch(m, points.data(), result.data(), points.size());
math::ref::TransformPointsBatch(m, points.data(), result.data(), points.size()); // mêmes valeurs
```

## Fonctionnalités réalisées

Socle obligatoire :

- [x] Audit de la bibliothèque, **5 bugs de convention corrigés** dans `Mat4` (`MultiplyVector`, `TRS`, `ExtractRotation`, `ValidTRS`, `Perspective`) avec tests de non-régression
- [x] Conventions documentées (stockage, vecteurs, composition, vecteur nul, alignement)
- [x] 3 traitements par lots : référence C++ + SIMD SSE/SSE2, lots vides / petits / non multiples de 4, aucun accès hors limites
- [x] Comparaison AoS / SoA sur les 3 traitements, conversions mesurées séparément et coût total mesuré
- [x] Fonction ASM x64 (`Dot3Asm`, `DotBatchAsm`) appelée depuis C++ et testée
- [x] Deux extraits de désassemblage commentés (référence optimisée et SIMD explicite)
- [x] Tests automatisés : résultats connus, vecteurs nuls, tailles limites, identité, translation, rotation, composition, tolérances absolues et relatives justifiées
- [x] Application console : choix du traitement, de la version et de la taille, données reproductibles, paramètres de relance affichés
- [x] Mesures Release x64 : 4 tailles, médiane, dispersion, rapport référence / SIMD, résultats bruts, reproductibilité vérifiée (2 exécutions)
- [x] Vérification de ce que produit le compilateur (`/Qvec-report:2`, `dumpbin`)
- [ ] **Analyse au profiler CPU de Visual Studio : protocole écrit (rapport § 10), à réaliser et compléter dans l'IDE**

Extensions facultatives :

- [x] Normalisation approchée (`_mm_rsqrt_ps`) comparée à la référence, erreur quantifiée et testée
- [x] Produit de matrices 4 × 4 en SSE (unitaire et par lots)
- [x] Auto-vectorisation : un cas réussi (SoA) et un cas bloqué (AoS) expliqués
- [x] Version SIMD naïve (1 point / registre) comparée à la version 4 points

## Résultats principaux

AMD Ryzen 7 9800X3D, MSVC 19.38, n = 100 000, ns par élément (accélération / référence AoS) :

| Traitement | Référence C++ | SSE AoS | SSE SoA | SSE SoA + conversions |
|---|---|---|---|---|
| Produit scalaire | 0,605 | 0,266 (**×2,3**) | 0,219 (×2,8) | 0,887 (×0,7) |
| Normalisation | 3,449 | 0,705 (**×4,9**) | 0,585 (×5,9) | 1,274 (×2,7) |
| Transformation | 1,167 | 0,515 (**×2,3**) | 0,341 (×3,4) | 1,041 (×1,1) |
| ASM (produit scalaire, scalaire) | 0,605 | 0,604 (×1,0) | – | – |

À retenir : la normalisation (racine + 3 divisions) profite le plus du SIMD ; en AoS la transposition limite le gain à ×2,3 ; convertir des données AoS en SoA pour un seul calcul coûte plus que ce qu'il fait gagner ; la référence SoA est auto-vectorisée par le compilateur et égale (voire bat) le SSE explicite. Analyse complète : [`docs/REPORT.md`](docs/REPORT.md).

## Limites

- SSE/SSE2 uniquement (pas de variante AVX).
- Résultats identiques au bit près entre référence et SIMD seulement avec `/fp:precise` et sans FMA.
- Normalisation : composantes > ~1e19 non supportées, composantes < ~1e-19 traitées comme un vecteur nul.
- Mesures sur une seule machine ; données « chaudes » en cache pour n ≤ 100 000.
- La documentation Doxygen de `docs/html` date de GTech 2 ; la régénérer avec `doxygen Doxyfile` pour inclure les nouveaux fichiers.
- Voir le rapport § 12 pour la liste complète.

## Sources

Intel Intrinsics Guide ; Microsoft Learn (convention d'appel x64, MASM `ml64`, `/arch`, `/fp`, messages du vectoriseur `/Qvec-report`, framework de tests C++, `VirtualAlloc`/`VirtualProtect`) ; N. Higham, *Accuracy and Stability of Numerical Algorithms* (bornes d'erreur flottante) ; guide utilisateur de Google Benchmark (méthodologie) ; cours ASM/SIMD GTech 3.

## Utilisation de l'IA

**Claude Code (Anthropic)** a été utilisé comme assistant pour l'audit, l'écriture et la simplification du code (SIMD, MASM, tests, benchmark) et la rédaction de la documentation. Les mesures proviennent d'exécutions réelles (résultats bruts joints) et les extraits assembleur de `dumpbin` sur le binaire compilé. Détails : rapport § 15.

## Ancien projet : Asteroids (GTech 2)

Le dossier `SFML_Project1` (projet *Game* de la solution) contient le jeu Asteroids réalisé en GTech 2 avec SFML 3.0.2 et la même bibliothèque mathématique (auteurs : Najim BAKKALI et Tristan GERMAIN). Il n'est pas concerné par ce module.
