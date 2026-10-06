#pragma once
#include <cstdint>

namespace bench
{
    /**
     * @brief Verifie toutes les versions (ref et SIMD, AoS et SoA) avant les mesures.
     *
     * 1. Resultats connus calcules a la main (produits scalaires, vecteur nul, NaN, identite,
     *    translation, rotation, composition, produit matriciel).
     * 2. Donnees aleatoires de tailles 0..33 et quelques grandes tailles non multiples de 4 :
     *    chaque resultat est compare a un calcul en double avec une borne d'erreur theorique
     *    (pas seulement a l'autre version : l'accord de deux versions ne prouve pas leur justesse).
     * 3. Acces hors limites : les tableaux AoS sont colles a une page memoire interdite, les sorties
     *    SoA sont suivies de valeurs sentinelles qui ne doivent pas etre modifiees.
     *
     * @return true si tout est correct.
     */
    bool RunValidation(std::uint32_t seed);
}
