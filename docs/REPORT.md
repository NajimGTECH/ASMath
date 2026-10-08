# Rapport technique – ASM / SIMD, bibliothèque mathématique

GTech 3 – Spécialisation Moteur – projet du 21/09 au 09/10/2026.
Auteur : Najim BAKKALI. Machine de mesure : AMD Ryzen 7 9800X3D, Windows 11, MSVC 19.38 (Visual Studio, toolset v143).

Documents liés : [README](../README.md) (compilation, utilisation, conventions), [désassemblage commenté](DISASSEMBLY.md), résultats bruts dans [`Benchmark/results/`](../Benchmark/results).

---

## 1. Objectif et périmètre

Partir de la bibliothèque mathématique de GTech 2 (Vector2/3, Mat3/4, Quaternion), la **vérifier**, puis **optimiser trois traitements par lots** en comparant une version C++ de référence à une version SIMD explicite (intrinsics SSE/SSE2) :

| Traitement | Entrée | Sortie |
|---|---|---|
| Produit scalaire par lots | deux tableaux de `Vector3<float>` | tableau de `float` |
| Normalisation par lots | un tableau de `Vector3<float>` | tableau de `Vector3<float>` |
| Transformation de points | une matrice affine `Mat4<float>` + un tableau de points | tableau de points (w = 1, pas de division perspective) |

S'y ajoutent : une fonction en **assembleur x64** (produit scalaire), la comparaison **AoS / SoA**, des **tests automatisés**, une **application console de benchmark** et ce rapport.
Extensions facultatives réalisées (après le socle) : normalisation approchée (`rsqrt`) avec erreur quantifiée, produit Mat4 × Mat4 en SSE, comparaison d'une version SIMD « naïve » et d'une version « 4 points par itération ».

## 2. État initial de la bibliothèque (audit)

La bibliothèque existante (templates `Vector3<T>`, `Mat4<T>`, stockage `T m[4][4]` en row-major) compilait et ses tests passaient, mais la lecture du code a montré que **la convention des vecteurs n'était pas appliquée de façon cohérente**. `MultiplyPoint`, `Translate`, `Rotate`, `LookAt`, `Ortho` utilisaient des **vecteurs lignes** (`p' = p * M`, translation dans la ligne 3), alors que d'autres fonctions utilisaient la convention inverse. Les tests existants ne le voyaient pas car ils n'utilisaient que des matrices diagonales ou des cas symétriques.

| Fonction | Problème trouvé | Correction | Test de non-régression |
|---|---|---|---|
| `Mat4::MultiplyVector` | utilisait `m[0][1], m[0][2]` pour x' (convention colonne) : une direction tournée partait dans le mauvais sens | même formule que `MultiplyPoint`, sans la ligne 3 | `TestMultiplyVectorUsesRowVectors` |
| `Mat4::TRS` | calculait `R * S`, soit « tourner puis mettre à l'échelle selon les axes du monde » : faux dès que l'échelle n'est pas uniforme | `S * R` puis translation (ordre S → R → T) | `TestTRSOrderWithNonUniformScale` |
| `Mat4::ExtractRotation` | formule de manuel écrite pour des vecteurs colonnes, sans transposition : renvoyait la **rotation inverse** | lecture de la matrice transposée, division de chaque ligne par son échelle | `TestExtractRotationRoundTrip` |
| `Mat4::ValidTRS` | testait l'orthogonalité des colonnes au lieu des lignes (axes) | utilise les lignes 0, 1, 2 | `TestTRSOrderWithNonUniformScale` |
| `Mat4::Perspective` | le `-1` et le terme de profondeur étaient inversés (disposition colonne) | `m[2][3] = -1`, `m[3][2] = 2fn/(n-f)` | `TestPerspectiveNearAndFarPlanes` (near → -1, far → +1) |
| `Vector3::Normalize` | ne modifiait pas un vecteur NaN alors que `Normalized()` renvoie (0,0,0) | même règle que `Normalized()` | `NormalizeBatchTests::ZeroVectorGivesZero` |
| `Mat4.inl`, `Vector3.h` | avertissements de conversion `double → float`, caractères mal encodés | casts explicites, ASCII | – |

Le reste du socle (`Dot`, `Normalized`, `operator*` de Mat4, `MultiplyPointAffine`) était correct et a été conservé tel quel : la référence des traitements par lots **réutilise directement l'API existante**.

Le premier essai de SIMD (commit `a4ba169`) a été **réécrit** : commentaires en français, validation de 600 lignes dans l'application de benchmark, dépendance à nanobench. Il est remplacé par un code plus simple à expliquer, des tests MSTest séparés et un petit outil de mesure écrit à la main (§ 7).

## 3. Conventions de l'API

| Sujet | Choix |
|---|---|
| Stockage des matrices | **row-major** : `m[ligne][colonne]`, 16 floats contigus, une ligne = 16 octets (un registre SSE) |
| Convention des vecteurs | **vecteurs lignes** : `p' = p * M`. Lignes 0-2 = images des axes X, Y, Z ; ligne 3 = translation ; colonne 3 = (0,0,0,1) pour une matrice affine |
| Ordre de composition | `A * B` applique **A puis B**. Transformation d'objet : `Scale * Rotate * Translate` (identique à DirectX) |
| Points / directions | points : w = 1 (`MultiplyPoint`, `MultiplyPointAffine`) ; directions : w = 0 (`MultiplyVector`) |
| Transformation par lots | w = 1 implicite, **pas de division perspective**, colonne 3 ignorée (matrice supposée affine) |
| Vecteur nul | `Normalized()` renvoie **(0, 0, 0)** si la longueur n'est pas strictement positive : vecteur nul, composante NaN, ou vecteur si petit que x²+y²+z² vaut 0 en float (composantes < ~1e-19). Au-delà de ~1e19 la longueur déborde (infini) : non supporté |
| Taille des lots | `n` quelconque : 0 (aucun accès mémoire, pointeurs nuls acceptés), petit, non multiple de 4 |
| Alignement | **aucun requis**. Un tableau de `Vector3<float>` n'est aligné que sur 4 octets (sizeof = 12) ; tout le SIMD utilise `_mm_loadu_ps` / `_mm_storeu_ps`. Les `std::vector<float>` du SoA sont alignés sur 16 octets par l'allocateur MSVC x64, mais le code n'en dépend pas |
| Aliasing | entrée et sortie peuvent être le même tableau (traitement en place), pas de recouvrement partiel |
| Fichiers | `Vector3Batch.h` (dot, normalisation, SoA, conversions), `Mat4Batch.h` (transformation, Mat4 × Mat4), `AsmFunctions.h` (assembleur). Namespaces `math::ref` (référence) et `math::simd` (SSE), mêmes signatures |

Les `static_assert` de `Vector3Batch.h` / `Mat4Batch.h` vérifient à la compilation que `Vector3<float>` fait 12 octets et `Mat4<float>` 64 octets sans padding : c'est ce qui permet de voir un tableau de vecteurs comme un tableau de floats.

## 4. Organisation mémoire et contenu des lanes

Un registre SSE (`__m128`) contient 4 floats, appelés *lanes* (lane 0 à 3). Une instruction `ps` (*packed single*) fait la même opération sur les 4 lanes.

### AoS (Array of Structures) – la disposition naturelle de `std::vector<Vector3<float>>`

```
mémoire : x0 y0 z0 | x1 y1 z1 | x2 y2 z2 | x3 y3 z3 | x4 ...
```

Charger 16 octets donne `[x0 y0 z0 x1]` : un mélange de deux vecteurs, inutilisable directement. Deux stratégies ont été implémentées :

1. **« Une lane = un vecteur »** (`simd::DotBatch`, `simd::NormalizeBatch`, `simd::TransformPointsBatch`) : 4 vecteurs = 12 floats = exactement 3 registres. On les charge (aucune lecture au-delà des 4 vecteurs) puis on les **transpose** avec 5 `shufps` (`LoadTranspose4`, `src/SseHelpers.h`) :

   ```
   r0 = [x0 y0 z0 x1]          X = [x0 x1 x2 x3]
   r1 = [y1 z1 x2 y2]   ==>    Y = [y0 y1 y2 y3]
   r2 = [z2 x3 y3 z3]          Z = [z0 z1 z2 z3]
   ```

   Lane k = vecteur i+k. Ensuite `X*X + Y*Y + Z*Z` calcule 4 produits scalaires avec 3 `mulps` et 2 `addps`. Pour écrire un résultat AoS, l'opération inverse (`StoreTranspose4`) coûte 2 `unpck` + 6 `shufps`.

2. **« Une lane = une composante »** (`simd::TransformPointsBatchNaive`) : le registre contient `[x' y' z' w']` d'un seul point, calculé comme `x*Ligne0 + y*Ligne1 + z*Ligne2 + Ligne3` (les lignes de la matrice sont contiguës en row-major). Pas de transposition, mais la lane w' est calculée pour rien (25 % du travail perdu) et il faut diffuser x, y, z (`_mm_set1_ps`).

### SoA (Structure of Arrays) – `struct Vec3SoA { std::vector<float> x, y, z; }`

```
x : x0 x1 x2 x3 x4 ...     y : y0 y1 y2 y3 ...     z : z0 z1 z2 z3 ...
```

Un `_mm_loadu_ps(&x[i])` donne directement `X = [xi xi+1 xi+2 xi+3]` : **aucune transposition**. C'est aussi une disposition que le compilateur sait auto-vectoriser (§ 9).

### Gestion du reste

Toutes les boucles SIMD tournent tant que `i + 4 <= n`, puis une boucle scalaire (le même code que la référence) traite les `n % 4` derniers éléments. Il n'y a donc jamais de lecture ou d'écriture hors de `[0, n)`. C'est vérifié par des tests avec page mémoire protégée (§ 6).

## 5. Versions mesurées

| Opération | Version | Description |
|---|---|---|
| `dot` | `ref-aos` | **référence** : `out[i] = a[i].Dot(b[i])` |
| | `simd-aos` | SSE, 4 vecteurs / itération, transposition |
| | `asm-aos` | boucle écrite en assembleur x64 (scalaire) |
| | `ref-soa` / `simd-soa` | même calcul sur données SoA |
| | `simd-soa+conv` | conversion AoS → SoA des deux entrées **+** `simd-soa` (coût total si les données sont en AoS) |
| `normalize` | `ref-aos` | **référence** : `out[i] = in[i].Normalized()` |
| | `simd-aos` | SSE, `sqrtps` + `divps`, masque pour le vecteur nul (pas de branche) |
| | `ref-soa` / `simd-soa` | sur données SoA |
| | `simd-soa+conv` | AoS → SoA + `simd-soa` + SoA → AoS |
| | `simd-aos-approx` | *(extension)* `rsqrtps` (inverse de racine approché) |
| `transform` | `ref-aos` | **référence** : `out[i] = M.MultiplyPointAffine(in[i])` |
| | `simd-aos` | SSE, 4 points / itération (12 coefficients diffusés) |
| | `simd-aos-naive` | SSE, 1 point / itération (lanes = x', y', z', w') |
| | `ref-soa` / `simd-soa` / `simd-soa+conv` | comme ci-dessus |
| `convert` | `aos-to-soa`, `soa-to-aos` | conversions seules, mesurées séparément |
| `mat4mul` | `ref`, `simd` | *(extension)* produits de deux tableaux de matrices |

## 6. Validation de la justesse

**Principe** : l'accord entre deux versions ne prouve pas qu'elles sont justes (elles peuvent avoir le même bug). Chaque version est donc vérifiée de trois façons indépendantes (`UnitTests/BatchTests.cpp`, 102 tests au total, tous au vert).

1. **Résultats connus calculés à la main**, choisis pour être exacts en float (petits entiers, puissances de 2, `(3,4,0)` → `(0.6, 0.8, 0)` car 5 est exact et la division est correctement arrondie) : comparaison **exacte**. Matrice identité, translation, mise à l'échelle, rotation de 90° autour de Z, composition dans les deux ordres (`T*R` ≠ `R*T`), colonne 3 ignorée.
2. **Données aléatoires comparées au même calcul en double**, avec une borne d'erreur **justifiée** au lieu d'un epsilon arbitraire.
3. **Comparaison ref / SIMD** : les opérations sont faites dans le même ordre (`(x*x' + y*y') + z*z'`, `sqrt` et division IEEE), avec `/fp:precise` et sans FMA, donc les valeurs doivent être **identiques** (`==`). C'est le cas pour toutes les versions exactes, sur toutes les tailles testées et dans le benchmark (colonne `check = identical`).

### Tolérances

On note u = 2⁻²⁴ ≈ 5,96·10⁻⁸ l'erreur relative maximale d'**une** opération float (arrondi au plus proche). Une somme de k produits arrondis a une erreur bornée par γ(k)·Σ|termes| avec γ(k) = k·u / (1 − k·u) (N. Higham, *Accuracy and Stability of Numerical Algorithms*, chap. 3). Le calcul en double sert de valeur « exacte » (erreur ~10⁻¹⁶, négligeable devant u).

| Calcul | Tolérance | Justification |
|---|---|---|
| Produit scalaire | \|f − d\| ≤ γ(3)·(\|ax·bx\| + \|ay·by\| + \|az·bz\|) | 3 produits, 2 additions. Borne **absolue** proportionnelle à la taille des termes : reste valable quand le résultat est proche de 0 (annulation) |
| Normalisation | \|f − d\| ≤ 4u par composante | longueur² : γ(3), la racine divise l'erreur par 2 et ajoute u, la division ajoute u → ~3,5u relatif ; les composantes d'un vecteur unitaire sont ≤ 1 |
| Transformation | \|f − d\| ≤ γ(4)·Σ\|termes\| par composante | 4 termes (3 produits + translation), 3 additions |
| Rotations (sin/cos) | max(1e-6 absolu, 1e-6 relatif) | float(π/2) est faux de ~4,4·10⁻⁸ donc cos(π/2) ≈ −4,4·10⁻⁸ au lieu de 0 ; la chaîne quaternion → matrice → point fait ~10 opérations (≈ 6·10⁻⁷). L'absolu sert près de 0, le relatif pour les grandes valeurs |
| Normalisation approchée | 1,5·2⁻¹² + 8u relatif | erreur relative max de `rsqrtps` donnée par Intel (≈ 3,7·10⁻⁴) + arrondis |

### Cas limites testés

Lot vide (avec pointeurs nuls), toutes les tailles de 0 à 40 (tous les restes 0 à 3, plusieurs blocs) et 1000-1003, vecteurs nuls placés dans un bloc SIMD et dans le reste, NaN, vecteur dont la longueur² sous-déborde, traitement en place (`in == out`), matrice avec colonne 3 non nulle.

### Détection des accès hors limites

`GuardedArray` (`UnitTests/TestHelpers.h`) place le **dernier élément du tableau juste avant une page mémoire `PAGE_NOACCESS`** (`VirtualAlloc` + `VirtualProtect`). Lire ou écrire un seul octet après la fin provoque immédiatement une violation d'accès, au lieu de lire silencieusement des données au hasard. Les sorties SoA (`std::vector`) sont suivies de valeurs sentinelles qui ne doivent pas changer.

**Vérification que le test détecte vraiment l'erreur** : en remplaçant temporairement `i + 4 <= n` par `i < n` dans `simd::DotBatch` (lecture de 16 octets sur le dernier `Vector3`), le test `NoOutOfBoundsAccess` échoue avec `Exception Code: C0000005` à la ligne fautive. Le code a ensuite été remis en état.

## 7. Protocole de mesure

| Élément | Choix |
|---|---|
| Machine | AMD Ryzen 7 9800X3D (8 cœurs, 96 Mo de cache L3), Windows 11 |
| Compilateur | MSVC 19.38.33145, Release x64 |
| Options (MathsLib et Benchmark) | `/O2` (auto-vectorisation autorisée), `/fp:precise`, jeu d'instructions x64 par défaut (**SSE2, pas d'AVX ni de FMA**), pas de `/GL` |
| Jeu d'instructions du code SIMD | SSE / SSE2 uniquement (le CPU supporte AVX2 et AVX-512, non utilisés) |
| Lancement | sans débogueur (le programme affiche un avertissement sinon), même machine, mêmes entrées pour toutes les versions |
| Données | générées avant toute mesure, reproductibles : `std::mt19937` (algorithme fixé par la norme) et conversion en float faite à la main, graine 42. Composantes uniformes dans [−100, 100), matrice TRS aléatoire |
| Tailles | 10 (petit, non multiple de 4), 1 000 (cache L1/L2), 100 000 (L2/L3), 4 000 000 (48 Mo par tableau AoS) ; Mat4 : 10, 1 000, 100 000 |
| Échauffement | 50 ms d'appels non mesurés par version (caches, prédicteur de branchement, montée en fréquence) |
| Calibration | nombre d'appels doublé jusqu'à ce qu'un échantillon dure ≥ 2 ms (la résolution de `steady_clock` = 100 ns devient négligeable) |
| Échantillons | 31 par version et par taille ; temps en **ns par élément** |
| Statistiques | **médiane**, quartiles Q1/Q3, **dispersion = (Q3 − Q1) / médiane** (IQR %), min, max ; **accélération = médiane référence / médiane version** |
| Exclusions | aucune allocation pendant la mesure (tous les tampons sont préparés avant), aucun affichage, conversions AoS/SoA préparées hors chronométrage (sauf dans les versions `+conv` où elles font partie de ce qu'on mesure) |
| Résultats utilisés | la fonction mesurée est appelée via un **pointeur de fonction lu dans une variable `volatile`** : le compilateur ne peut ni l'inliner ni supprimer l'appel. Une somme de contrôle des sorties est écrite dans `summary.csv` |
| Entrées modifiées ? | non : les sorties vont dans des tableaux séparés, il n'y a rien à réinitialiser entre deux essais |
| Vérification | avant de mesurer une version, sa sortie est comparée à celle de la référence sur les mêmes données |

Commande pour relancer exactement la même campagne (affichée par le programme) :

```
Benchmark.exe --suite --seed 42 --samples 31 --out results
```

### Biais connus et limites du protocole

- **Données chaudes dans le cache** : le même lot est traité en boucle. Pour n ≤ 100 000, les données restent dans le cache (L3 de 96 Mo sur ce CPU), ce qui avantage le calcul pur. La taille 4 000 000 (≈ 100 à 300 Mo selon l'opération) montre le cas limité par la mémoire.
- **Coût d'appel** : pour n = 10, l'appel indirect et le prologue de la fonction (sauvegarde des registres XMM) pèsent beaucoup ; c'est un biais identique pour toutes les versions, mais il réduit les écarts.
- **Fréquence variable** (boost du CPU) et autres processus : limités par l'échauffement et la médiane, non supprimés.
- **Reproductibilité mesurée** : la campagne a été lancée deux fois. Sur 86 médianes, l'écart moyen entre les deux exécutions est de **1,5 %**, l'écart maximal de **13,6 %** (lots de 4 000 000, limités par la mémoire, et conversions à n = 10). **Des écarts de moins de ~10 % à n = 4 000 000 ne sont donc pas significatifs.** Aucune conclusion ci-dessous ne change d'une exécution à l'autre (résultats de la 2ᵉ exécution dans `Benchmark/results/run2/`).

## 8. Résultats

Médiane en **ns par élément** (accélération par rapport à `ref-aos` entre parenthèses). Dispersion (IQR) : en général < 2 %, au plus 13 % (détail dans `summary.csv`).

### Produit scalaire

| version | n = 10 | n = 1000 | n = 100000 | n = 4000000 |
|---|---|---|---|---|
| `ref-aos` | 0.693 (**×1.00**) | 0.599 (**×1.00**) | 0.605 (**×1.00**) | 0.653 (**×1.00**) |
| `simd-aos` | 0.365 (**×1.90**) | 0.264 (**×2.27**) | 0.266 (**×2.27**) | 0.378 (**×1.73**) |
| `asm-aos` | 0.652 (**×1.06**) | 0.586 (**×1.02**) | 0.604 (**×1.00**) | 0.650 (**×1.00**) |
| `ref-soa` | 0.488 (**×1.42**) | 0.155 (**×3.87**) | 0.219 (**×2.76**) | 0.340 (**×1.92**) |
| `simd-soa` | 0.462 (**×1.50**) | 0.151 (**×3.97**) | 0.219 (**×2.76**) | 0.328 (**×1.99**) |
| `simd-soa+conv` | 1.403 (**×0.49**) | 0.780 (**×0.77**) | 0.887 (**×0.68**) | 1.716 (**×0.38**) |

- **SIMD AoS : ×2,3** et non ×4 : les 10 shuffles de transposition coûtent plus que le calcul (voir [DISASSEMBLY.md](DISASSEMBLY.md)).
- **ASM : ×1,0**. La boucle écrite à la main contient les mêmes instructions scalaires que celles générées par le compilateur. Pas de gain, résultat attendu.
- **SoA : ×3,9 à n = 1000**, et la référence C++ SoA est aussi rapide que le SSE explicite car **le compilateur l'auto-vectorise** (§ 9).
- **Régression : la conversion AoS → SoA coûte plus cher que tout le calcul** (0,30 ns/élément par tableau converti, deux tableaux). Si les données sont stockées en AoS, passer en SoA juste pour ce calcul est **plus lent que la référence** (×0,4 à ×0,8). Le SoA n'est intéressant que si les données sont **stockées** en SoA.
- **À n = 4 000 000 les gains diminuent** (×1,7 à ×2) : le calcul (2 multiplications-additions par 24 octets lus) est limité par la bande passante mémoire, plus par le calcul.

### Normalisation

| version | n = 10 | n = 1000 | n = 100000 | n = 4000000 |
|---|---|---|---|---|
| `ref-aos` | 3.600 (**×1.00**) | 3.448 (**×1.00**) | 3.449 (**×1.00**) | 3.479 (**×1.00**) |
| `simd-aos` | 1.052 (**×3.42**) | 0.704 (**×4.89**) | 0.705 (**×4.89**) | 0.715 (**×4.86**) |
| `ref-soa` | 4.122 (**×0.87**) | 3.886 (**×0.89**) | 3.898 (**×0.88**) | 4.040 (**×0.86**) |
| `simd-soa` | 1.150 (**×3.13**) | 0.574 (**×6.01**) | 0.585 (**×5.90**) | 0.606 (**×5.74**) |
| `simd-soa+conv` | 2.796 (**×1.29**) | 1.235 (**×2.79**) | 1.274 (**×2.71**) | 1.928 (**×1.80**) |
| `simd-aos-approx` | 0.603 (**×5.97**) | 0.477 (**×7.23**) | 0.477 (**×7.24**) | 0.501 (**×6.94**) |

- **Meilleur gain du projet : ×4,9 en AoS, ×6 en SoA.** La référence fait pour chaque vecteur 1 `sqrtss` et **3 `divss`** (instructions lentes, plusieurs cycles de débit chacune) plus une branche (`len > 0`) ; la version SSE fait 1 `sqrtps` et 3 `divps` pour **4 vecteurs** et remplace la branche par un masque (`cmpgt` + `and`). Le calcul est assez lourd pour que la transposition devienne secondaire : on approche le ×4 théorique, et le dépasse légèrement car la branche et l'appel de secours `sqrtf` de la référence disparaissent.
- On voit dans le désassemblage de la référence un `call sqrtf` après `sqrtss` : avec `/fp:precise`, MSVC appelle la fonction de la CRT quand le résultat est NaN (racine d'un négatif), pour positionner `errno`. Ce chemin n'est jamais pris ici mais ajoute un test par vecteur.
- **Régression non expliquée en détail : `ref-soa` est ~12 % plus lent que `ref-aos`**, alors que les deux boucles contiennent les mêmes instructions de calcul (aucune n'est vectorisée, raison 1305). Hypothèse non vérifiée : 6 flux mémoire séparés (3 lectures, 3 écritures) au lieu de 2. À confirmer au profiler.
- **Avec conversions**, la SoA reste gagnante (×2,7) car le calcul est cher, mais moins que `simd-aos` (×4,9) : **pour ce traitement, mieux vaut rester en AoS avec transposition dans les registres.**
- *(Extension)* la version approchée (`rsqrtps`) gagne encore ×1,5 sur `simd-aos` mais avec une erreur maximale mesurée de **2,6·10⁻⁴** sur une composante (bornée par 3,7·10⁻⁴ en relatif, testée). Acceptable pour des directions d'éclairage, pas pour un moteur physique qui renormalise des vecteurs en boucle (l'erreur s'accumule).

### Transformation de points

| version | n = 10 | n = 1000 | n = 100000 | n = 4000000 |
|---|---|---|---|---|
| `ref-aos` | 1.354 (**×1.00**) | 1.170 (**×1.00**) | 1.167 (**×1.00**) | 1.181 (**×1.00**) |
| `simd-aos` | 1.009 (**×1.34**) | 0.513 (**×2.28**) | 0.515 (**×2.27**) | 0.531 (**×2.22**) |
| `simd-aos-naive` | 0.704 (**×1.92**) | 0.674 (**×1.74**) | 0.678 (**×1.72**) | 0.681 (**×1.73**) |
| `ref-soa` | 0.934 (**×1.45**) | 0.316 (**×3.70**) | 0.317 (**×3.69**) | 0.375 (**×3.15**) |
| `simd-soa` | 0.916 (**×1.48**) | 0.343 (**×3.41**) | 0.341 (**×3.43**) | 0.389 (**×3.04**) |
| `simd-soa+conv` | 1.923 (**×0.70**) | 1.006 (**×1.16**) | 1.041 (**×1.12**) | 1.716 (**×0.69**) |

- **SIMD AoS 4 points : ×2,3**. Limite identifiée dans le désassemblage : 12 constantes diffusées + les données dépassent les **16 registres XMM** ; le compilateur range 5 constantes sur la pile et les relit à chaque tour (*register spilling*), et le prologue sauvegarde `xmm6`–`xmm15`.
- **Version naïve (1 point / itération) : ×1,7**, moins bonne sur les grands lots (25 % de lanes perdues, diffusion de x, y, z) mais **meilleure à n = 10** : elle n'a besoin que de 4 registres (les lignes de la matrice), son prologue est presque vide. Pour de très petits lots, la version « simple » est la bonne.
- **SoA : la référence auto-vectorisée (×3,7) bat légèrement notre SSE explicite (×3,4)** de façon reproductible (0,316 contre 0,343 ns, écart 8 % dans les deux exécutions). Le compilateur génère une boucle aussi large que la nôtre ; notre version souffre du même manque de registres. **L'intrinsic n'est pas automatiquement meilleur que le compilateur.**
- **Conversions** : comme pour le produit scalaire, le coût AoS → SoA → AoS annule le gain (×0,7 à ×1,2).

### Conversions seules et Mat4 × Mat4

| version | n = 10 | n = 1000 | n = 100000 | n = 4000000 |
|---|---|---|---|---|
| `aos-to-soa` | 0.505 | 0.298 | 0.321 | 0.453 |
| `soa-to-aos` | 0.495 | 0.354 | 0.354 | 0.428 |

| *(extension)* Mat4 × Mat4 | n = 10 | n = 1000 | n = 100000 |
|---|---|---|---|
| `ref` (`operator*`) | 11.476 (×1.00) | 11.105 (×1.00) | 11.200 (×1.00) |
| `simd` | 2.610 (**×4.40**) | 2.636 (**×4.21**) | 2.759 (**×4.06**) |

Une conversion coûte ~0,3 ns par vecteur, soit **autant qu'un produit scalaire complet** : c'est la mesure qui explique toutes les régressions `+conv`. Le produit de matrices SSE (une ligne du résultat = combinaison des 4 lignes de B, 4 `mulps` + 3 `addps`) donne des résultats identiques à `operator*` et un gain de ×4, car la référence (triple boucle avec `Zero()` initial) n'est pas vectorisée.

### Synthèse

| Traitement | Meilleure version (données AoS) | Gain | Facteur limitant |
|---|---|---|---|
| Produit scalaire | `simd-aos` | ×2,3 | shuffles de transposition ; mémoire pour les grands lots |
| Normalisation | `simd-aos` | ×4,9 | division / racine : calcul lourd, la vectorisation paie |
| Transformation | `simd-aos` (grands lots), `simd-aos-naive` (n ≈ 10) | ×2,3 / ×1,9 | registres XMM insuffisants (spilling) |

Si les données étaient **stockées** en SoA : ×2,8 à ×6, mais la référence C++ auto-vectorisée fait presque aussi bien que les intrinsics pour le produit scalaire et la transformation.

## 9. Ce que produit le compilateur (auto-vectorisation)

Le projet MathsLib est compilé avec `/Qvec-report:2`, qui indique pour chaque boucle si elle est vectorisée et sinon pourquoi (codes documentés par Microsoft, *Vectorizer and parallelizer messages*) :

| Boucle | Résultat | Explication |
|---|---|---|
| `ref::DotBatch` (AoS) | non vectorisée, raison **1200** | dépendances possibles entre itérations : `out` pourrait recouvrir `a`/`b`, et les accès AoS ont un pas de 3 floats. Le compilateur déroule ×4 en scalaire à la place |
| `ref::DotBatchSoA` | **vectorisée** | accès contigus de pas 1. Le compilateur ajoute au début **6 tests de recouvrement** à l'exécution (`out` contre chacun des 6 tableaux d'entrée) puis une boucle `movups`/`mulps`/`addps` équivalente à la nôtre |
| `ref::TransformPointsBatchSoA` | **vectorisée** | idem (explique pourquoi elle égale ou bat `simd-soa`) |
| `ref::NormalizeBatch`, `ref::NormalizeBatchSoA`, `ref::TransformPointsBatch` | non vectorisées, raison **1305** | informations de type insuffisantes : la boucle manipule des `Vector3` (structures) et, pour la normalisation, contient une branche et un appel de secours `sqrtf` |
| `AoSToSoA` | non vectorisée, raison **1501** | aliasing possible sur un tableau de structures |
| boucles SIMD principales (`i += 4`) | raison **502** | normal : le pas n'est pas +1, ce sont déjà nos boucles vectorielles |

Cas réussi et cas bloqué (extension « approfondir l'auto-vectorisation ») : le **même calcul** de produit scalaire est vectorisé en SoA et pas en AoS. La disposition des données décide de ce que le compilateur peut faire, plus que le code de calcul lui-même.

## 10. Analyse au profiler CPU

> **À COMPLÉTER** – cette partie doit être faite dans l'interface de Visual Studio (le profiler n'est pas utilisable depuis la ligne de commande dans les conditions du projet). Protocole prévu :
>
> 1. Configuration **Release x64**, menu *Déboguer > Profileur de performances* (Alt+F2), outil **Utilisation du processeur**.
> 2. Arguments de lancement : `--op normalize --sizes 100000 --samples 101` (la normalisation est le traitement au plus grand gain, ~3 s de mesure).
> 3. Dans le rapport : vue *Fonctions*, comparer le temps inclusif de `math::ref::NormalizeBatch`, `math::simd::NormalizeBatch` et `math::ref::NormalizeBatchSoA` ; double-cliquer pour voir les lignes chaudes (attendu : les `divss` / `sqrtss` dans la référence, `divps` dans la version SSE).
> 4. Refaire avec `--op dot --version simd-soa+conv --sizes 100000` pour voir la part du temps passée dans `AoSToSoA` par rapport à `simd::DotBatchSoA` (attendu d'après les mesures : ~75 % dans la conversion).
> 5. Vérifier l'hypothèse sur `ref-soa` plus lent que `ref-aos` pour la normalisation (§ 8).
>
> Ajouter ici : captures, pourcentages observés, et si les observations confirment ou contredisent les explications des § 8 et 9.

## 11. Écarts numériques

- Toutes les versions « exactes » (SSE, SoA, ASM, Mat4 × Mat4) donnent des valeurs **identiques** à la référence, vérifié sur toutes les tailles de test et sur les 4 000 000 d'éléments du benchmark. Condition : `/fp:precise`, même ordre des opérations, pas de FMA. Avec `/fp:fast` ou `/arch:AVX2` (contraction en FMA possible), les résultats pourraient différer de quelques ulp : les tests contre le calcul en double resteraient valables (bornes γ(k)), mais pas le test d'égalité.
- `+0` et `−0` peuvent différer entre `Mat4::operator*` (qui part de `0 + ...`) et la version SSE ; la comparaison `==` les considère égaux, comme IEEE 754.
- Normalisation approchée : erreur max mesurée 2,6·10⁻⁴ (borne théorique 3,7·10⁻⁴ relative).
- Erreur des versions exactes par rapport au calcul en double : toujours sous les bornes γ(k) (testé), soit quelques 10⁻⁷ en relatif.

## 12. Limites

- Code SIMD limité à **SSE/SSE2** (4 lanes). AVX (8 lanes, instructions à 3 opérandes qui suppriment les copies `movaps`) n'a pas été implémenté.
- La transposition AoS est faite à chaque appel ; une bibliothèque de moteur physique aurait intérêt à **stocker** les données en SoA (ou AoSoA) dès le départ.
- Le manque de registres dans `simd::TransformPointsBatch` pourrait être réduit (par exemple en ne diffusant pas la translation, ou en traitant la matrice ligne par ligne), non tenté.
- La détection hors limites par page protégée couvre les tableaux AoS ; pour les `Vec3SoA` (`std::vector`), seules les écritures après `n` sont détectées (sentinelles), pas les lectures.
- Les vecteurs aux composantes > ~10¹⁹ (longueur infinie) ne sont pas gérés par la normalisation ; les composantes < ~10⁻¹⁹ sont traitées comme un vecteur nul (documenté et testé).
- Mesures sur **une seule machine** (Zen 5) ; les rapports entre versions peuvent changer sur un CPU Intel ou plus ancien (latence des divisions, des shuffles).
- Profiler : voir § 10.

## 13. Résultats bruts

`Benchmark/results/` :

- `run_info.txt` – CPU, jeux d'instructions, compilateur, options, graine, commande de relance ;
- `summary.csv` – une ligne par (opération, taille, version) : médiane, Q1, Q3, min, max, IQR %, accélération, nombre d'appels par échantillon, résultat de la vérification, somme de contrôle ;
- `samples.csv` – les 31 échantillons bruts de chaque mesure (2 666 lignes) ;
- `console_output.txt` – la sortie console complète ;
- `run2/` – la deuxième exécution complète utilisée pour estimer la reproductibilité.

## 14. Sources

- Intel, *Intrinsics Guide* – sémantique et précision de `_mm_shuffle_ps`, `_mm_rsqrt_ps` (erreur relative ≤ 1,5·2⁻¹²), etc.
- Microsoft Learn : *x64 calling convention* ; *MASM for x64 (ml64.exe)* ; */arch (x64)* ; */fp (Specify floating-point behavior)* ; *Vectorizer and parallelizer messages* (codes 502, 1200, 1305, 1501) ; *Write unit tests for C/C++ (Microsoft Unit Testing Framework)* ; *VirtualAlloc / VirtualProtect*.
- N. J. Higham, *Accuracy and Stability of Numerical Algorithms*, 2ᵉ éd., SIAM, 2002, chap. 3 (bornes γ(k) d'erreur des sommes et produits scalaires).
- Google Benchmark, *User guide* – principes d'échauffement, répétitions, empêcher l'élimination du calcul.
- Cours ASM / SIMD GTech 3 (squelette de fonction MASM).

## 15. Utilisation de l'IA générative

Conformément au cahier des charges, usage significatif signalé : **Claude Code (Anthropic)** a été utilisé comme assistant pour :

- l'audit de la bibliothèque (repérage des incohérences de convention vecteurs lignes / colonnes) ;
- la rédaction et la simplification du code SIMD, de la fonction MASM, des tests et de l'outil de benchmark ;
- la mise en forme de ce rapport, du README et du désassemblage commenté.

Toutes les mesures proviennent d'exécutions réelles du programme sur la machine indiquée (résultats bruts joints), et chaque affirmation sur le code machine provient de la sortie de `dumpbin` du binaire compilé. Le code a été relu, compris et testé ; chaque fonction peut être expliquée et modifiée lors de la revue de code.
