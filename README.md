# ASMath – Bibliothèque mathématique ASM / SIMD

Projet GTech 3 (spécialisation moteur) – premier module du projet fil rouge « moteur physique ».
Projet réalisé par **Najim BAKKALI** et **Tristan GERMAIN**.

La bibliothèque mathématique de GTech 2 (`Vector2`, `Vector3`, `Mat3`, `Mat4`, `Quaternion`) a été **auditée et corrigée**, puis des traitements par lots ont été **optimisés en SIMD explicite (intrinsics SSE/SSE2)** et comparés à leur **référence C++** :

1. produits scalaires entre deux tableaux de `Vector3<float>` ;
2. normalisation d'un tableau de `Vector3<float>` ;
3. transformation de points 3D par une même matrice affine `Mat4<float>` (w = 1, sans division perspective).

S'y ajoutent une **fonction en assembleur x64 (MASM)**, une comparaison **AoS / SoA**, des **tests automatisés**, une **application console de benchmark**, et en extension des versions SSE pour `Vector2` et `Mat3`.

### Point de départ : notre bibliothèque de GTech 2

Comme le demande le cahier des charges, nous n'avons pas réécrit la bibliothèque mathématique : nous avons **repris celle de notre projet de GTech 2**, un jeu Asteroids en C++ / SFML réalisé par nous deux. Elle contenait déjà les templates `Vector2<T>`, `Vector3<T>`, `Mat3<T>`, `Mat4<T>` et la classe `Quaternion`, avec leur documentation Doxygen (`docs/html`) et leurs tests. Elle a été importée dans ce dépôt au commit `ce0af99` (*Restore old math library*).

Cette bibliothèque sert de **référence C++** : nous l'avons vérifiée, corrigée là où c'était nécessaire (voir l'audit ci-dessous), puis complétée uniquement avec ce qui manquait pour le module (traitements par lots, versions SSE, fonction ASM). Le jeu lui-même ne fait pas partie de ce module et a été retiré de la solution.

📄 **Rapport technique** : [`docs/REPORT.md`](docs/REPORT.md) (audit, organisation mémoire, protocole, mesures détaillées, limites).
📄 **Désassemblage commenté** : [`docs/DISASSEMBLY.md`](docs/DISASSEMBLY.md).
📊 **Résultats bruts** : [`Benchmark/results/`](Benchmark/results).

---

## Ce qu'on a fait, en résumé

| Étape | Ce qui a été réalisé |
|---|---|
| **Audit de la bibliothèque** | Lecture de tout `Mat4` / `Vector3`. La convention des vecteurs n'était pas cohérente : **5 bugs corrigés** dans `Mat4` (`MultiplyVector`, `TRS`, `ExtractRotation`, `ValidTRS`, `Perspective`) + `Vector3::Normalize` sur un vecteur NaN. Chaque correction a son test de non-régression |
| **Conventions** | Stockage row-major, vecteurs lignes (`p' = p * M`), ordre de composition, comportement du vecteur nul, alignement : tout est documenté (voir plus bas) |
| **3 traitements par lots** | Pour chacun : une version `math::ref` (C++ qui réutilise l'API existante) et une version `math::simd` (SSE/SSE2), en AoS et en SoA, avec conversions AoS ↔ SoA |
| **Gestion des restes** | Boucle SIMD de 4 en 4, puis boucle scalaire pour les `n % 4` derniers éléments : lot vide, petits lots et tailles quelconques acceptés, aucun accès hors limites |
| **Assembleur x64** | `Dot3Asm` (produit scalaire d'un `Vector3`) et `DotBatchAsm` (même calcul en boucle), écrits en MASM et appelés depuis C++ (`extern "C"`) |
| **Désassemblage** | Deux extraits commentés (référence optimisée par le compilateur et version SIMD explicite) + comparaison avec notre fonction ASM |
| **Tests** | 102 tests MSTest : valeurs connues, comparaison à un calcul en double avec tolérances justifiées, vecteurs nuls, NaN, tailles 0 à 40 et 1000-1003, identité, translation, rotation, composition, détection des accès hors limites (page mémoire protégée) |
| **Benchmark** | Application console : choix du traitement, de la version et de la taille, données reproductibles (graine fixe), échauffement, 31 échantillons, médiane, quartiles, dispersion, accélération, vérification des résultats, export CSV |
| **Extensions** | Normalisation approchée (`rsqrt`) avec erreur mesurée, Mat4 × Mat4 en SSE, version SIMD « naïve » vs « 4 points », auto-vectorisation (cas réussi / cas bloqué), versions SSE de `Vector2` (dot, scale, lerp) et `Mat3` (déterminant, transposée, produit) |

Répartition du travail (voir l'historique Git) : Najim – audit, traitements `Vector3` / `Mat4` (SIMD, AoS/SoA), fonction ASM, tests, application de benchmark, documentation ; Tristan – `Vector2` et `Mat3` (versions classiques et SSE, AoS et SoA). Les deux parties ont ensuite été réunies dans la même application de benchmark.

## Structure du dépôt

```
SFML_Jour2.sln                 solution Visual Studio (MathsLib, UnitTests, Benchmark)
MathsLib/                      bibliothèque statique (C++20)
  include/
    Vector2.h, Vector3.h,      socle existant (templates), conventions documentées dans Mat4.h
    Mat3.h, Mat4.h, Quaternion.h
    Vector3Batch.h             produit scalaire / normalisation par lots, Vec3SoA, conversions AoS <-> SoA
    Mat4Batch.h                transformation de points par lots, Mat4 x Mat4
    AsmFunctions.h             déclarations des fonctions assembleur (extern "C")
  src/
    Vector3Batch.cpp           versions math::ref (C++) et math::simd (SSE)
    Mat4Batch.cpp
    SseHelpers.h               transposition AoS <-> SoA dans les registres
    asm/Dot3.asm               produit scalaire écrit en assembleur x64 (MASM)
  Vector2SSE.h                 Vector2 : dot, scale, lerp en SSE (AoS et SoA)
  Mat3SSE.h                    Mat3 : déterminant, transposée, produit en SSE (AoS et SoA)
UnitTests/                     tests MSTest C++ (Explorateur de tests de Visual Studio)
  UnitTests.cpp                tests du socle + régressions de l'audit
  BatchTests.cpp               tests des traitements par lots, de l'ASM et des cas limites
  TestHelpers.h                tolérances justifiées, données aléatoires, tableaux à page protégée
Benchmark/                     application console de mesure
  src/main.cpp                 menu interactif + ligne de commande
  src/Workloads.cpp            liste des traitements et de leurs versions
  src/Measure.cpp              échauffement, calibration, échantillons, médiane / quartiles
  src/SystemInfo.cpp           CPU, jeux d'instructions, compilateur et options
  results/                     résultats bruts (CSV + sortie console)
docs/                          rapport, désassemblage commenté (+ ancienne doc Doxygen html)
  profiler/                    traces du profiler CPU (.diagsession), instructions chaudes, expérience
```

## Compilation

Prérequis : **Visual Studio 2022 ou plus récent**, charge de travail *Développement Desktop en C++* (contient MSVC, MASM `ml64` et le framework de tests C++). Windows x64.

1. Ouvrir `SFML_Jour2.sln`.
2. Choisir la configuration **Release | x64** (les mesures en Debug n'ont aucun sens).
3. *Générer > Générer la solution*.

En ligne de commande (*Developer PowerShell for VS*) :

```bat
msbuild SFML_Jour2.sln -p:Configuration=Release -p:Platform=x64
```

Options utilisées pour MathsLib et Benchmark (Release) : `/O2` (auto-vectorisation autorisée), `/fp:precise`, jeu d'instructions x64 par défaut (**SSE2**, pas d'AVX, pas de FMA), pas de `/GL`, `/Qvec-report:2` sur MathsLib (rapport d'auto-vectorisation). `Dot3.asm` est assemblé par `ml64` grâce à la personnalisation de build MASM du projet.

## Lancer les tests

- Visual Studio : *Test > Explorateur de tests > Exécuter tout* (102 tests).
- Ligne de commande :

  ```bat
  vstest.console.exe x64\Release\UnitTests.dll /Platform:x64
  ```

Chaque version (référence, SSE, SoA, ASM) est vérifiée contre des **valeurs connues**, contre un **calcul en double avec une borne d'erreur justifiée**, et contre les autres versions. L'accord entre deux versions ne suffit pas : elles pourraient avoir le même bug. Détails : rapport § 6.

## Lancer les benchmarks

Toujours en **Release x64, sans débogueur** (Ctrl+F5 dans Visual Studio). Le programme affiche un avertissement si un débogueur est attaché. Exécutable : `x64\Release\Benchmark.exe`.

```bat
Benchmark.exe                                   menu interactif : traitement, version, taille du lot
Benchmark.exe --list                            liste des traitements et versions
Benchmark.exe --op dot --version simd-aos --sizes 1000
Benchmark.exe --op normalize --version ref-aos,simd-aos,simd-soa --sizes 10,100000
Benchmark.exe --suite --seed 42 --samples 31 --out results    campagne complète (~30 s)
```

| Option | Rôle (défaut) |
|---|---|
| `--op` | `dot`, `normalize`, `transform`, `convert`, `mat4mul`, `vec2dot`, `vec2scale`, `vec2lerp`, `mat3det`, `mat3transpose`, `mat3mul` |
| `--version` | une ou plusieurs versions séparées par des virgules, ou `all` (défaut). La référence est toujours mesurée |
| `--sizes` | tailles de lots (défaut : 10, 1000, 100000, 4000000 ; matrices : 10, 1000, 100000) |
| `--seed` | graine des données aléatoires (42) |
| `--samples` | nombre d'échantillons chronométrés (31) |
| `--out` | dossier des résultats bruts (`summary.csv`, `samples.csv`, `run_info.txt`) |

Pour chaque version, le programme affiche la **médiane** (ns par élément), les **quartiles**, la **dispersion (IQR %)**, l'**accélération** (temps référence / temps version) et le **résultat de la vérification** par rapport à la référence (`identical`, ou `approx` avec l'écart maximal).

Protocole : données générées avant la mesure (graine fixe), 50 ms d'échauffement, calibration pour que chaque échantillon dure au moins 2 ms, 31 échantillons, aucune allocation ni affichage pendant la mesure, appel par pointeur de fonction `volatile` pour que le calcul ne puisse pas être supprimé. Détails : rapport § 7.

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
| Alignement | **aucun requis** (chargements non alignés `loadu`) ; un `Vector3<float>` fait 12 octets, aligné sur 4 |
| Aliasing | entrée et sortie peuvent être le même tableau (en place), pas de recouvrement partiel |
| Versions | `math::ref::X` (référence C++) et `math::simd::X` (SSE) ont la même signature ; `math::x64asm` pour l'assembleur |

## Résultats obtenus (console, Release x64)

Machine : **AMD Ryzen 7 9800X3D**, Windows 11, MSVC 19.38, `/O2 /fp:precise`, SSE2 uniquement. Campagne du 11/10/2026 lancée sans débogueur (`Benchmark.exe --suite --seed 42 --samples 31 --out results`). Temps en **ns par élément** (médiane de 31 échantillons) et accélération par rapport à la référence AoS. La dispersion (IQR) est en général sous 3 %.

### Les trois traitements obligatoires (n = 100 000)

| Traitement | Référence C++ | SSE AoS | SSE SoA | SSE SoA + conversions | ASM (scalaire) |
|---|---|---|---|---|---|
| Produit scalaire | 0,594 | 0,266 (**×2,2**) | 0,215 (×2,8) | 0,879 (×0,7) | 0,590 (×1,0) |
| Normalisation | 3,473 | 0,710 (**×4,9**) | 0,583 (×6,0) | 1,298 (×2,7) | – |
| Transformation de points | 1,177 | 0,512 (**×2,3**) | 0,313 (×3,8) | 1,025 (×1,15) | – |

Effet de la taille du lot (version SSE AoS) :

| Traitement | n = 10 | n = 100 000 | n = 4 000 000 |
|---|---|---|---|
| Produit scalaire | ×1,9 | ×2,2 | ×1,7 |
| Normalisation | ×3,4 | ×4,9 | ×4,9 |
| Transformation | ×1,35 (version naïve : ×1,9) | ×2,3 | ×2,2 |

Toutes ces versions donnent des résultats **identiques au bit près** à la référence (colonne `check = identical`).

### Analyse

- **La normalisation est le meilleur cas (×4,9).** La référence fait, pour chaque vecteur, une racine carrée et trois divisions, qui sont des instructions lentes. La version SSE fait une seule racine `sqrtps` et trois `divps` pour **4 vecteurs à la fois**, et remplace le test `if (longueur > 0)` par un masque, donc sans branchement. Quand le calcul est lourd, le coût de la réorganisation des données devient négligeable. Le gain dépasse même le ×4 théorique : le profiler a montré que la référence perd l'essentiel de son temps à recopier un `Vector3` temporaire (voir plus bas). Contre une référence scalaire sans ce défaut, le gain dû au SIMD seul est d'environ ×3,3.
- **Le produit scalaire et la transformation ne gagnent que ×2,2 et ×2,3 en AoS.** Les données d'un tableau de `Vector3` sont rangées `x y z x y z …`. Pour mettre 4 `x` dans un registre il faut les **transposer** avec des `shufps`, et ces shuffles coûtent presque autant que le calcul lui-même. Pour la transformation, il faut garder les 12 coefficients de la matrice dans des registres : x64 n'a que 16 registres XMM, donc le compilateur en range une partie sur la pile (*spilling*).
- **La disposition SoA est la plus rapide (×2,8 à ×6,0)**, car un simple `loadu` charge 4 `x` d'un coup, sans transposition. Mais **la référence C++ en SoA est presque aussi rapide que nos intrinsics** : le compilateur l'auto-vectorise tout seul (`/Qvec-report:2`). Écrire des intrinsics n'est donc pas toujours mieux que de bien ranger ses données.
- **Convertir AoS → SoA juste pour un calcul est une régression** (×0,7 pour le produit scalaire). Une conversion coûte environ 0,3 ns par vecteur, autant que le produit scalaire lui-même. Le SoA n'est intéressant que si les données sont **stockées** en SoA dès le départ.
- **L'assembleur écrit à la main n'apporte rien (×1,0)** : notre boucle contient les mêmes instructions scalaires que celles générées par le compilateur en `/O2`. Le gain vient de la **vectorisation** (4 éléments par instruction), pas du fait d'écrire de l'assembleur.
- **Petits lots (n = 10)** : le coût fixe de l'appel et de la sauvegarde des registres pèse beaucoup. Pour la transformation, la version naïve (1 point par registre, seulement 4 registres pour la matrice) bat la version 4 points.
- **Très grands lots (n = 4 000 000, ~48 Mo par tableau)** : les données ne tiennent plus dans le cache. Pour le produit scalaire (peu de calcul pour beaucoup d'octets lus), on est limité par la **bande passante mémoire** et le gain tombe à ×1,7. La normalisation, elle, reste limitée par le calcul (×4,9).
- **Extensions** : normalisation approchée `rsqrtps` ×7,2 avec une erreur maximale mesurée de 2,6·10⁻⁴ ; Mat4 × Mat4 en SSE ×5,0 (13,8 → 2,7 ns par produit).

### Vector2 et Mat3 (extension, n = 100 000)

| Opération | Référence AoS | SSE AoS (un élément à la fois) | SSE SoA (4 éléments à la fois) |
|---|---|---|---|
| `Vector2` dot (somme) | 0,389 | 0,577 (**×0,7**) | 0,386 (×1,0) |
| `Vector2` scale | 0,661 | 0,471 (×1,4) | 0,203 (**×3,3**) |
| `Vector2` lerp | 0,848 | 0,473 (×1,8) | 0,214 (**×4,0**) |
| `Mat3` déterminant (somme) | 1,057 | 1,056 (×1,0) | 0,385 (**×2,7**) |
| `Mat3` transposée | 1,583 | 1,385 (×1,1) | 0,650 (×2,4) |
| `Mat3` produit | 3,727 | 3,534 (×1,05) | 1,302 (**×2,9**) |

- **SSE sur un seul élément = peu ou pas de gain, voire une perte.** Un `Vector2` n'occupe que 2 lanes sur 4, et une `Mat3` a des lignes de 3 floats. Il faut construire le registre (`_mm_set_ps`), calculer, puis ressortir le résultat : pour `Vector2::Dot` cela fait **plus d'instructions que le calcul scalaire** (×0,7). Le `Mat3SSE::Determinant` AoS est du code scalaire, d'où le ×1,0.
- **SSE en SoA (4 éléments à la fois) = vrai gain (×2,4 à ×4,0).** C'est la même leçon que pour `Vector3` : on vectorise **entre les éléments**, pas à l'intérieur d'un élément.
- **Sommes (dot, déterminant)** : la boucle `result += …` est une chaîne de dépendances (chaque addition attend la précédente). `DotSoA` ajoute les 4 lanes une par une dans `result`, donc la chaîne reste aussi longue qu'en scalaire : pas de gain (×1,0). Une amélioration possible serait de garder un accumulateur `__m128` et de ne sommer les 4 lanes qu'à la fin. Le déterminant SoA additionne les 4 lanes dans un ordre différent de la référence, d'où `approx (max diff 9,2·10⁻⁴)` sur une somme de 100 000 déterminants qui vaut −539 : c'est un écart d'arrondi relatif de 2·10⁻⁶, dû à l'ordre des additions, pas une erreur de calcul.
- `Mat3` produit, référence SoA : ×0,53, plus lente que l'AoS. Hypothèse non vérifiée : les 9 tableaux de sortie sont des `std::vector<float>` comme les entrées, donc le compilateur ne peut pas prouver qu'ils ne se recouvrent pas et relit les entrées après chaque écriture.

### Analyse au profiler CPU

Trois lancements du benchmark ont été profilés avec le collecteur CPU de Visual Studio (`VSDiagnostics.exe`, 4 000 échantillons par seconde). Les traces ont été analysées par fonction, puis instruction par instruction. Détails : rapport § 10, traces dans [`docs/profiler/`](docs/profiler).

| Ce qu'on pensait | Ce que le profiler montre | Verdict |
|---|---|---|
| Le produit scalaire `+conv` passe ~75 % de son temps à convertir | `AoSToSoA` = **76 %**, calcul = 24 % | confirmé |
| La normalisation `+conv` passe ~50 % de son temps à convertir | conversions = **53 %**, calcul = 47 % | confirmé |
| La version SSE passe son temps dans la division/racine et la transposition | division/racine ~34 %, shuffles ~22 %, copies de registres ~16 % | confirmé |
| La référence de normalisation passe son temps dans la racine et les 3 divisions | **95 % sur la recopie d'un `Vector3` temporaire** créé par le ternaire de `Normalized()` | **contredit** |

La dernière ligne est le résultat le plus intéressant. `Normalized()` construit son résultat dans un vecteur temporaire sur la pile (3 écritures de 4 octets), puis le relit avec une lecture de 8 octets. Le processeur ne peut pas transmettre directement une donnée vers une lecture qui chevauche deux écritures (*store-to-load forwarding*), donc il attend. Nous l'avons vérifié avec une petite expérience : la même normalisation écrite avec un `if` et sans temporaire passe de **3,48 à 2,31 ns par élément (−34 %)**, avec des résultats identiques au bit près. Une partie du ×4,9 de la version SSE vient donc de ce défaut de la référence. Le gain dû au SIMD seul est d'environ ×3,3. La référence n'a pas été modifiée, pour rester le point de comparaison du projet.

### Reproductibilité

La campagne a été lancée plusieurs fois (08/10, puis 11/10 après la fusion des parties Vector2 / Mat3). Sur les 86 médianes communes, l'écart moyen entre deux campagnes est d'environ 4 %. Les écarts les plus grands (jusqu'à ~30 %) concernent n = 4 000 000, qui est limité par la mémoire et donc plus sensible à l'état de la machine. Une exception reste non expliquée : la référence `mat4mul` est ~24 % plus lente le 11/10 que le 08/10 alors que son code n'a pas changé (hypothèse : placement du code en mémoire après recompilation). **Aucune conclusion ne change d'une campagne à l'autre.**

## Est-ce que l'optimisation est pertinente pour un moteur de jeu ?

**Oui, mais seulement pour les traitements par lots sur beaucoup de données, et à condition de choisir la bonne disposition mémoire.**

### Pourquoi

Un jeu à 60 images par seconde a **16,6 ms par image** pour tout faire : gameplay, physique, animation, rendu. Un moteur physique refait les mêmes calculs sur des milliers d'objets à chaque image : intégrer les positions, passer les points des colliders du repère local au repère monde, normaliser des normales de contact, etc. Ce sont exactement nos traitements par lots.

Avec nos mesures, pour 100 000 éléments par image :

| Traitement | Référence | SSE AoS | Gagné par image |
|---|---|---|---|
| Normalisation | 347 µs | 71 µs | 276 µs |
| Transformation de points | 118 µs | 51 µs (SoA : 31 µs) | 67 µs |
| Produit scalaire | 59 µs | 27 µs | 32 µs |
| **Total** | **524 µs (3,1 % de l'image)** | **149 µs (0,9 %)** | **375 µs** |

Gagner ~0,4 ms par image, c'est du temps rendu au reste du moteur (plus d'objets, plus d'itérations du solveur physique). Le gain est **réel et mesuré**, et les résultats sont identiques au bit près, ce qui est important en physique : une simulation doit donner le même résultat entre deux versions du code.

### Quand ça ne vaut pas le coup

- **Calcul isolé sur un seul vecteur ou une seule matrice** (un `Vector2::Dot`, un `Mat3` à la fois) : peu ou pas de gain, parfois une perte. Le SIMD sert à traiter **plusieurs éléments en même temps**.
- **Petits lots** (une dizaine d'éléments) : le gain existe mais le temps total est déjà négligeable (quelques dizaines de nanosecondes).
- **Convertir ses données en SoA juste pour un calcul** : plus lent que de ne rien faire. La disposition doit être choisie **dès la conception** (par exemple dans un système de composants où chaque composante est stockée dans son propre tableau).
- **Normalisation approchée** : très rapide (×7), mais une erreur de 10⁻⁴ qui s'accumule si on renormalise en boucle. Acceptable pour de l'éclairage ou des particules décoratives, pas pour la physique.

### Comment l'utiliser : exemple dans un moteur physique

```cpp
#include "Vector3Batch.h"
#include "Mat4Batch.h"

// Un collider stocke ses points dans son repère local.
// A chaque image, on les passe dans le repère monde avec la matrice de l'objet.
struct ConvexCollider
{
    std::vector<math::Vec3f> localPoints;
    std::vector<math::Vec3f> worldPoints;   // même taille que localPoints
};

void UpdateCollider(ConvexCollider& collider, const math::Mat4f& worldMatrix)
{
    // 4 points par itération en SSE, le reste en scalaire : n quelconque accepté
    math::simd::TransformPointsBatch(worldMatrix,
                                     collider.localPoints.data(),
                                     collider.worldPoints.data(),
                                     collider.localPoints.size());
}

// Après la détection des collisions : normaliser toutes les normales de contact d'un coup.
// Le traitement en place (in == out) est autorisé ; une normale nulle donne (0, 0, 0).
void NormalizeContactNormals(std::vector<math::Vec3f>& normals)
{
    math::simd::NormalizeBatch(normals.data(), normals.data(), normals.size());
}
```

Pour un système de particules dont les positions sont **stockées** en SoA (`math::Vec3SoA`), on utiliserait directement `math::simd::TransformPointsBatchSoA`, sans conversion : c'est la configuration la plus rapide mesurée (×3,8). La version `math::ref` reste disponible avec la même signature, ce qui permet de comparer ou de déboguer facilement.

## Fonctionnalités réalisées

Socle obligatoire :

- [x] Audit de la bibliothèque, **5 bugs de convention corrigés** dans `Mat4` avec tests de non-régression
- [x] Conventions documentées (stockage, vecteurs, composition, vecteur nul, alignement)
- [x] 3 traitements par lots : référence C++ + SIMD SSE/SSE2, lots vides / petits / non multiples de 4, aucun accès hors limites
- [x] Comparaison AoS / SoA sur les 3 traitements, conversions mesurées séparément et coût total mesuré
- [x] Fonction ASM x64 (`Dot3Asm`, `DotBatchAsm`) appelée depuis C++ et testée
- [x] Deux extraits de désassemblage commentés (référence optimisée et SIMD explicite)
- [x] Tests automatisés : résultats connus, vecteurs nuls, tailles limites, identité, translation, rotation, composition, tolérances absolues et relatives justifiées
- [x] Application console : choix du traitement, de la version et de la taille, données reproductibles, paramètres de relance affichés
- [x] Mesures Release x64 : 4 tailles, médiane, dispersion, rapport référence / SIMD, résultats bruts, reproductibilité vérifiée
- [x] Vérification de ce que produit le compilateur (`/Qvec-report:2`, `dumpbin`)
- [x] Analyse au profiler CPU (collecteur de Visual Studio) : 3 lancements, analyse par fonction et par instruction, hypothèse vérifiée par une expérience (rapport § 10)

Extensions facultatives :

- [x] Normalisation approchée (`_mm_rsqrt_ps`) comparée à la référence, erreur quantifiée et testée
- [x] Produit de matrices 4 × 4 en SSE (unitaire et par lots)
- [x] Auto-vectorisation : un cas réussi (SoA) et un cas bloqué (AoS) expliqués
- [x] Version SIMD naïve (1 point / registre) comparée à la version 4 points
- [x] `Vector2` (dot, scale, lerp) et `Mat3` (déterminant, transposée, produit) en SSE, AoS et SoA, mesurés dans le benchmark

## Limites

- SSE/SSE2 uniquement (pas de variante AVX).
- `Vector3::Normalized()` (référence) est ralentie de ~34 % par un `Vector3` temporaire (vu au profiler) ; volontairement non corrigée pour garder la même référence.
- Résultats identiques au bit près entre référence et SIMD seulement avec `/fp:precise` et sans FMA.
- Normalisation : composantes > ~1e19 non supportées, composantes < ~1e-19 traitées comme un vecteur nul.
- Les versions `Vector2SSE` / `Mat3SSE` sont vérifiées par le benchmark (comparaison à la référence) mais n'ont pas encore de tests unitaires dédiés.
- Mesures sur une seule machine (AMD Zen 5) ; données « chaudes » dans le cache pour n ≤ 100 000.
- La documentation Doxygen de `docs/html` date de GTech 2 ; la régénérer avec `doxygen Doxyfile` pour inclure les nouveaux fichiers.
- Voir le rapport § 12 pour la liste complète.

## Sources

Intel Intrinsics Guide ; Microsoft Learn (convention d'appel x64, MASM `ml64`, `/arch`, `/fp`, messages du vectoriseur `/Qvec-report`, framework de tests C++, `VirtualAlloc`/`VirtualProtect`, profiler *Utilisation du processeur* et `VSDiagnostics.exe`, Windows Performance Toolkit / `xperf`) ; N. Higham, *Accuracy and Stability of Numerical Algorithms* (bornes d'erreur flottante) ; guide utilisateur de Google Benchmark (méthodologie) ; cours ASM/SIMD GTech 3.

## Utilisation de l'IA

Nous (Najim et Tristan) avons utilisé **Claude (Anthropic)** comme assistant pour :

- **mieux comprendre le cahier des charges** et l'objectif du projet ;
- **comprendre les fonctions intrinsèques** (SSE/SSE2) et ce qu'elles font sur les registres ;
- **accélérer la rédaction du code** ;
- **mettre en commun rapidement nos travaux** lors des merges, puis pousser vers le dépôt Git ;
- **aider à collecter et analyser les traces du profiler CPU** ;
- **aider à rédiger ce README** et le rapport.

Toutes les mesures viennent d'exécutions réelles du programme sur la machine indiquée (résultats bruts dans `Benchmark/results/`). Le code a été relu, compris et testé par nous ; chacun de nous doit pouvoir l'expliquer et le modifier pendant la revue de code.
