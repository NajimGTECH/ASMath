# Désassemblage commenté

Ce document commente deux extraits de code machine générés par MSVC pour le **produit scalaire par lots** (`DotBatch`) :

1. la **référence C++ optimisée** (`math::ref::DotBatch`, compilée en `/O2`, auto-vectorisation autorisée) ;
2. la **version SIMD explicite** (`math::simd::DotBatch`, intrinsics SSE).

Il compare ensuite ces deux extraits à la fonction écrite à la main en assembleur (`Dot3Asm` / `DotBatchAsm`, fichier `MathsLib/src/asm/Dot3.asm`).

## Comment obtenir ces extraits

Configuration : Release x64, MSVC 19.38.33145 (toolset v143), `/O2 /fp:precise`, jeu d'instructions par défaut (SSE2), **sans `/GL`** (sinon les `.obj` contiennent du code intermédiaire et non du code machine).

```bat
dumpbin /disasm /nologo MathsLib\x64\Release\Vector3Batch.obj > Vector3Batch.asm.txt
```

(`dumpbin` est dans `VC\Tools\MSVC\<version>\bin\Hostx64\x64`, ou directement dans l'« Invite de commandes développeur ».)
On peut aussi mettre un point d'arrêt dans Visual Studio et ouvrir **Déboguer > Fenêtres > Code machine** (Ctrl+Alt+D).

Rappels utiles pour la lecture :

| Élément | Signification |
|---|---|
| `rcx, rdx, r8, r9` | 4 premiers arguments (convention d'appel Microsoft x64) : ici `a`, `b`, `out`, `n` |
| `xmm0`…`xmm15` | registres SSE de 128 bits = 4 floats (4 *lanes*) |
| suffixe `ss` (`movss`, `mulss`, `addss`) | *scalar single* : une seule lane (lane 0) est utilisée |
| suffixe `ps` (`movups`, `mulps`, `addps`) | *packed single* : les 4 lanes travaillent en même temps |
| `movups` / `movaps` | chargement 16 octets non aligné / copie de registre (ou accès aligné) |
| `shufps x, y, imm` | lanes basses prises dans `x`, lanes hautes dans `y`, selon l'octet `imm` (= `_MM_SHUFFLE`) |

---

## Extrait 1 – Référence C++ optimisée : `math::ref::DotBatch`

Code source (`MathsLib/src/Vector3Batch.cpp`) :

```cpp
for (std::size_t i = 0; i < n; ++i)
    out[i] = a[i].Dot(b[i]);          // (a.x*b.x + a.y*b.y) + a.z*b.z
```

Le rapport `/Qvec-report:2` indique : `loop not vectorized due to reason '1200'` (dépendances possibles entre itérations : le compilateur ne peut pas prouver que `out` ne recouvre pas `a` ou `b`, et les accès AoS ont un pas de 12 octets). Il ne vectorise donc pas, mais **déroule la boucle 4 fois** (4 vecteurs par tour, toujours en scalaire).

Boucle principale (le premier vecteur du tour est commenté, les 3 autres sont identiques avec d'autres décalages) :

```asm
; Avant la boucle : r10 = a - b (écart entre les deux tableaux), rax = &b[i] + 16, rbx = &out[i] + 8
; rcx = nombre de tours de 4 vecteurs
0040: movss  xmm2, dword ptr [rax+r10-10h]  ; xmm2 = a[i].x       (adresse de b + écart = adresse de a)
0047: mulss  xmm2, dword ptr [rax-10h]      ; xmm2 = a.x * b.x    (2e opérande lu directement en mémoire)
004C: movss  xmm0, dword ptr [rax+r10-0Ch]  ; xmm0 = a.y
0053: mulss  xmm0, dword ptr [rax-0Ch]      ; xmm0 = a.y * b.y
0058: movss  xmm1, dword ptr [rax+r10-8]    ; xmm1 = a.z
005F: mulss  xmm1, dword ptr [rax-8]        ; xmm1 = a.z * b.z
0064: addss  xmm2, xmm0                     ; xmm2 = a.x*b.x + a.y*b.y
0068: addss  xmm2, xmm1                     ; xmm2 = (...) + a.z*b.z   -> même ordre que Vector3::Dot
006C: movss  dword ptr [rbx-8], xmm2        ; out[i] = xmm2
      ...                                   ; vecteurs i+1, i+2, i+3 : même schéma (xmm3, xmm0/1/2...)
00F4: add    rax, 30h                       ; b avance de 4 vecteurs (4 * 12 = 48 = 0x30 octets)
0105: add    rbx, 10h                       ; out avance de 4 floats (16 octets)
0109: sub    rcx, 1                         ; un tour de moins
010D: jne    0040                           ; on boucle tant que rcx != 0
; puis une boucle de reste (0 à 3 vecteurs) avec exactement les mêmes instructions, un vecteur par tour
```

**Ce qu'il faut retenir**

- Par vecteur : 6 lectures de 4 octets, 3 `mulss`, 2 `addss`, 1 écriture de 4 octets. **Une seule lane sur 4** des registres XMM est utilisée.
- Le déroulage ×4 réduit le coût de la boucle (compteur, saut) mais ne change pas le nombre d'opérations flottantes.
- Le compilateur utilise des opérandes mémoire (`mulss xmm2, [mem]`) pour éviter des chargements séparés.
- Aucun registre non volatil n'est sauvegardé (sauf `rbx`) : la fonction est légère.

---

## Extrait 2 – SIMD explicite : `math::simd::DotBatch`

Code source :

```cpp
for (; i + 4 <= n; i += 4)
{
    __m128 ax, ay, az, bx, by, bz;
    detail::LoadTranspose4(pa + 3 * i, ax, ay, az);   // 3 movups + 5 shufps
    detail::LoadTranspose4(pb + 3 * i, bx, by, bz);   // 3 movups + 5 shufps
    _mm_storeu_ps(out + i, Dot4(ax, ay, az, bx, by, bz)); // 3 mulps + 2 addps + 1 movups
}
```

Les constantes de `shufps` correspondent aux `_MM_SHUFFLE` du code : `9Eh` = `_MM_SHUFFLE(2,1,3,2)` (→ `[x2 y2 x3 y3]`), `49h` = `(1,0,2,1)` (→ `[y0 z0 y1 z1]`), `8Ch` = `(2,0,3,0)` (→ X), `D8h` = `(3,1,2,0)` (→ Y), `CDh` = `(3,0,3,1)` (→ Z).

```asm
; Prologue : xmm6..xmm9 sont NON VOLATILS dans la convention Windows x64 -> sauvegardés sur la pile
0019: movaps xmmword ptr [rsp+30h], xmm6
0022: movaps xmmword ptr [rsp+20h], xmm7
002E: movaps xmmword ptr [rsp+10h], xmm8
0034: movaps xmmword ptr [rsp], xmm9
; rcx = &a[i] + 32 octets, rax = b - a (écart), r11 = i, rdx = i + 4, r9 = n

0040: movups xmm0, xmmword ptr [rcx-10h]      ; a.r1 = [y1 z1 x2 y2]
0044: add    rdx, 4
0048: movups xmm7, xmmword ptr [rcx-20h]      ; a.r0 = [x0 y0 z0 x1]
004C: movups xmm2, xmmword ptr [rax+rcx-20h]  ; b.r0 = [x0 y0 z0 x1] de b
0051: movups xmm9, xmmword ptr [rcx]          ; a.r2 = [z2 x3 y3 z3]
0055: movups xmm3, xmmword ptr [rcx+rax]      ; b.r2
0059: movaps xmm4, xmm0                       ; copies : shufps écrase son 1er opérande,
005C: movaps xmm6, xmm2                       ; il faut donc dupliquer les registres encore utiles
005F: movaps xmm8, xmm7
0063: shufps xmm4, xmm9, 9Eh                  ; a.xy23 = [x2 y2 x3 y3]
0068: shufps xmm8, xmm0, 49h                  ; a.yz01 = [y0 z0 y1 z1]
006D: movups xmm0, xmmword ptr [rax+rcx-10h]  ; b.r1
0072: add    rcx, 30h                         ; 4 vecteurs plus loin (48 octets)
0076: shufps xmm7, xmm4, 8Ch                  ; a.X = [x0 x1 x2 x3]
007A: shufps xmm6, xmm0, 49h                  ; b.yz01
007E: movaps xmm1, xmm0
0081: movaps xmm5, xmm6
0084: shufps xmm1, xmm3, 9Eh                  ; b.xy23
0088: shufps xmm5, xmm1, 0D8h                 ; b.Y = [y0 y1 y2 y3]
008C: movaps xmm0, xmm8
0090: shufps xmm2, xmm1, 8Ch                  ; b.X
0094: shufps xmm0, xmm4, 0D8h                 ; a.Y
0098: mulps  xmm5, xmm0                       ; yy = a.Y * b.Y   (4 produits d'un coup)
009B: mulps  xmm2, xmm7                       ; xx = a.X * b.X
009E: shufps xmm6, xmm3, 0CDh                 ; b.Z = [z0 z1 z2 z3]
00A2: shufps xmm8, xmm9, 0CDh                 ; a.Z
00A7: mulps  xmm6, xmm8                       ; zz = a.Z * b.Z
00AB: addps  xmm5, xmm2                       ; yy + xx  (l'addition IEEE est commutative : = xx + yy)
00AE: addps  xmm5, xmm6                       ; (xx + yy) + zz   -> même ordre que la référence
00B1: movups xmmword ptr [r8+r11*4], xmm5     ; out[i..i+3] en une seule écriture de 16 octets
00B6: add    r11, 4                           ; i += 4
00BA: cmp    rdx, r9                          ; i + 4 <= n ?
00BD: jbe    0040
; Épilogue : restauration de xmm6..xmm9, puis boucle de reste scalaire (identique à l'extrait 1)
```

**Ce qu'il faut retenir**

- Pour **4 vecteurs** : 6 chargements de 16 octets, **10 `shufps`**, 7 copies `movaps`, 3 `mulps`, 2 `addps`, 1 écriture. Le calcul utile (5 instructions) est 4 fois plus large qu'en scalaire, mais la **transposition AoS → SoA** (shuffles + copies) représente la majorité des instructions. C'est le coût de la disposition AoS.
- Les 7 `movaps` existent parce que les instructions SSE n'ont que 2 opérandes (`shufps x, y` écrit dans `x`). Avec AVX (3 opérandes, `vshufps x, y, z`) elles disparaîtraient.
- Les 4 registres `xmm6`–`xmm9` doivent être sauvegardés/restaurés (convention d'appel Windows x64) : coût fixe par appel, visible sur les petits lots.
- La boucle de reste (`n % 4` éléments) est le même code scalaire que la référence : le compilateur l'a même déroulée, alors qu'elle ne fait jamais plus de 3 tours.
- Mesure (n = 100 000) : 0,266 ns/élément contre 0,605 pour la référence, soit **×2,27**, alors que le calcul est 4 fois plus large : les shuffles limitent le gain.

---

## Comparaison avec la fonction écrite à la main (`Dot3.asm`)

```asm
Dot3Asm PROC                            ; RCX = &a.x, RDX = &b.x
    movss   xmm0, dword ptr [rcx]       ; a.x
    mulss   xmm0, dword ptr [rdx]       ; a.x * b.x
    movss   xmm1, dword ptr [rcx + 4]   ; a.y
    mulss   xmm1, dword ptr [rdx + 4]   ; a.y * b.y
    addss   xmm0, xmm1
    movss   xmm1, dword ptr [rcx + 8]   ; a.z
    mulss   xmm1, dword ptr [rdx + 8]   ; a.z * b.z
    addss   xmm0, xmm1                  ; résultat dans XMM0 (valeur de retour float)
    ret
Dot3Asm ENDP
```

- C'est **exactement la même suite d'instructions** que le corps de la boucle du compilateur (extrait 1) : `movss` / `mulss` avec opérande mémoire / `addss`, dans le même ordre. Le résultat est donc identique au bit près (vérifié par le test `AsmTests::Dot3SameAsCpp` sur 10 000 paires).
- `DotBatchAsm` (boucle en assembleur) mesure **0,604 ns/élément** contre **0,605** pour la référence à n = 100 000 (×1,00) : écrire en assembleur une boucle scalaire n'apporte rien face à `/O2`. Le gain vient de la **vectorisation** (traiter 4 éléments par instruction), pas du fait d'écrire de l'assembleur.
- La fonction est une *leaf function* : elle n'appelle rien et n'utilise que des registres volatils (`rcx`, `rdx`, `r8`, `r9`, `xmm0`, `xmm1`), donc pas de prologue ni d'informations de déroulement (`PROC FRAME`).

---

## Observation complémentaire : manque de registres dans `simd::TransformPointsBatch`

La transformation de points diffuse 12 coefficients de la matrice dans 12 registres (`_mm_set1_ps`), et il faut en plus x, y, z, les résultats et les temporaires des shuffles. x64 n'a que **16 registres XMM** : le compilateur **range une partie des constantes sur la pile** (*register spilling*) et les relit à chaque tour :

```asm
0262: addps  xmm8, xmmword ptr [rsp+0C0h]   ; + m30 (translation x) relue en mémoire à chaque tour
027F: addps  xmm5, xmmword ptr [rsp+0D0h]   ; + m31
0291: addps  xmm4, xmmword ptr [rsp+0E0h]   ; + m32
02A6: movaps xmm4, xmmword ptr [rsp+0F0h]   ; rechargement d'une constante écrasée
02C2: movaps xmm5, xmmword ptr [rsp+100h]
```

Le prologue réserve aussi 0x1F8 octets de pile et sauvegarde `xmm6`–`xmm15`. Cela explique deux mesures du rapport :

- à n = 10, la version naïve « 1 point par itération » (4 registres de lignes seulement) est plus rapide que la version 4 points (prologue coûteux amorti sur 10 points seulement) ;
- en SoA, la référence auto-vectorisée par le compilateur (0,316 ns/élément) est un peu plus rapide que notre version SSE explicite (0,343 ns/élément).
