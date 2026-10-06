#include "Validation.h"
#include "Data.h"

#include <algorithm>
#include <bit>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace math;

namespace bench
{
    namespace
    {
        // =================================================================
        // Bornes d'erreur theoriques (Higham, "Accuracy and Stability of Numerical Algorithms")
        // u        = 2^-24 : erreur relative max d'un arrondi float (arrondi au plus proche).
        // gamma(k) = k*u / (1 - k*u) : borne pour une somme de k produits arrondis.
        // Ex. produit scalaire 3D : |fl(a.b) - a.b| <= gamma(3) * (|ax*bx| + |ay*by| + |az*bz|).
        // La reference "exacte" est calculee en double (erreur ~1e-16, negligeable devant u ~ 6e-8).
        // =================================================================
        constexpr double kU = FLT_EPSILON / 2.0;
        constexpr double Gamma(int k) { return k * kU / (1.0 - k * kU); }

        using Flat = std::vector<float>;
        using Variants = std::vector<std::pair<std::string, Flat>>; // (nom de la version, resultats a plat)

        // =================================================================
        // Statistiques par "traitement | version"
        // =================================================================
        struct Stats
        {
            std::size_t values = 0;
            std::size_t failures = 0;
            std::size_t compared = 0;   // valeurs comparees bit a bit a "ref AoS"
            std::size_t identical = 0;
            double worstRatio = 0.0;    // max(erreur / borne theorique)
            std::string firstFailure;
        };

        std::vector<std::pair<std::string, Stats>> g_stats; // ordre d'insertion conserve pour l'affichage

        Stats& StatsFor(const std::string& key)
        {
            for (auto& [k, s] : g_stats)
                if (k == key)
                    return s;
            g_stats.emplace_back(key, Stats{});
            return g_stats.back().second;
        }

        void Fail(const std::string& key, const std::string& message)
        {
            Stats& s = StatsFor(key);
            ++s.failures;
            if (s.firstFailure.empty())
                s.firstFailure = message;
        }

        // =================================================================
        // Detection des acces hors limites
        // =================================================================

        /**
         * Tableau dont le dernier element touche une page PAGE_NOACCESS : lire ou ecrire ne serait-ce
         * qu'un octet apres data[n-1] declenche une violation d'acces immediate. C'est exactement
         * l'erreur que ferait un _mm_loadu_ps de 16 octets sur le dernier Vector3 (12 octets).
         */
        template <typename T>
        class GuardedArray
        {
        public:
            explicit GuardedArray(std::size_t n) : m_size(n)
            {
                SYSTEM_INFO info;
                GetSystemInfo(&info);
                const std::size_t page = info.dwPageSize;
                const std::size_t bytes = n * sizeof(T);
                const std::size_t dataPages = (bytes + page - 1) / page;

                m_base = static_cast<std::byte*>(VirtualAlloc(nullptr, (dataPages + 1) * page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
                if (m_base == nullptr)
                    throw std::bad_alloc();

                DWORD oldProtect = 0;
                VirtualProtect(m_base + dataPages * page, page, PAGE_NOACCESS, &oldProtect);

                m_data = reinterpret_cast<T*>(m_base + dataPages * page - bytes);
                std::uninitialized_value_construct_n(m_data, n);
            }

            explicit GuardedArray(const std::vector<T>& src) : GuardedArray(src.size())
            {
                std::copy(src.begin(), src.end(), m_data);
            }

            ~GuardedArray() { VirtualFree(m_base, 0, MEM_RELEASE); }

            GuardedArray(const GuardedArray&) = delete;
            GuardedArray& operator=(const GuardedArray&) = delete;

            T* Data() { return m_data; }
            std::vector<T> ToVector() const { return std::vector<T>(m_data, m_data + m_size); }

        private:
            std::byte* m_base = nullptr;
            T* m_data = nullptr;
            std::size_t m_size = 0;
        };

        // __try/__except est interdit dans une fonction qui contient des objets C++ a detruire (C2712) :
        // on l'isole dans une fonction minimale appelee via un pointeur de fonction.
        bool CallCatchingAccessViolation(void (*fn)(void*), void* ctx)
        {
            __try
            {
                fn(ctx);
                return true;
            }
            __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                return false;
            }
        }

        template <typename F>
        void RunGuarded(const std::string& key, F&& f)
        {
            using Fn = std::remove_reference_t<F>;
            void* ctx = const_cast<void*>(static_cast<const void*>(std::addressof(f)));
            if (!CallCatchingAccessViolation([](void* p) { (*static_cast<Fn*>(p))(); }, ctx))
                Fail(key, "acces memoire hors limites (page de garde touchee)");
        }

        // Les sorties SoA (std::vector) sont suivies de kGuard valeurs sentinelles qui ne doivent pas changer.
        constexpr std::size_t kGuard = 4;
        constexpr float kSentinel = -12345.678f;

        Vec3SoA MakeSoAOutput(std::size_t n)
        {
            Vec3SoA s;
            s.x.assign(n + kGuard, kSentinel);
            s.y.assign(n + kGuard, kSentinel);
            s.z.assign(n + kGuard, kSentinel);
            return s;
        }

        void CheckSentinels(const std::string& key, const std::vector<float>& v, std::size_t n)
        {
            for (std::size_t i = n; i < v.size(); ++i)
                if (std::bit_cast<std::uint32_t>(v[i]) != std::bit_cast<std::uint32_t>(kSentinel))
                    Fail(key, "ecriture hors limites (sentinelle SoA modifiee)");
        }

        // =================================================================
        // Mise a plat des resultats (pour comparer AoS, SoA et matrices de la meme facon)
        // =================================================================
        Flat Flatten(const std::vector<Vec3f>& v)
        {
            Flat f;
            f.reserve(3 * v.size());
            for (const Vec3f& e : v) { f.push_back(e.x); f.push_back(e.y); f.push_back(e.z); }
            return f;
        }

        Flat Flatten(const Vec3SoA& s, std::size_t n)
        {
            Flat f;
            f.reserve(3 * n);
            for (std::size_t i = 0; i < n; ++i) { f.push_back(s.x[i]); f.push_back(s.y[i]); f.push_back(s.z[i]); }
            return f;
        }

        Flat Flatten(const std::vector<Mat4f>& v)
        {
            Flat f;
            f.reserve(16 * v.size());
            for (const Mat4f& mat : v)
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        f.push_back(mat.m[r][c]);
            return f;
        }

        // =================================================================
        // Execution de toutes les versions d'un traitement. Le premier element est toujours "ref AoS".
        // =================================================================
        Variants RunDot(const std::string& t, const std::vector<Vec3f>& a, const std::vector<Vec3f>& b)
        {
            const std::size_t n = a.size();
            GuardedArray<Vec3f> ga(a), gb(b);
            Vec3SoA sa(n), sb(n);
            ref::AoSToSoA(a.data(), sa, n);
            ref::AoSToSoA(b.data(), sb, n);

            auto aos = [&](const std::string& name, auto fn) {
                GuardedArray<float> out(n);
                RunGuarded(t + " | " + name, [&] { fn(ga.Data(), gb.Data(), out.Data(), n); });
                return std::pair{ name, out.ToVector() };
            };
            auto soa = [&](const std::string& name, auto fn) {
                Flat out(n + kGuard, kSentinel);
                fn(sa, sb, out.data(), n);
                CheckSentinels(t + " | " + name, out, n);
                out.resize(n);
                return std::pair{ name, out };
            };

            return { aos("ref  AoS", ref::DotBatch), aos("simd AoS", simd::DotBatch),
                     soa("ref  SoA", ref::DotBatchSoA), soa("simd SoA", simd::DotBatchSoA) };
        }

        Variants RunNormalize(const std::string& t, const std::vector<Vec3f>& v)
        {
            const std::size_t n = v.size();
            GuardedArray<Vec3f> gin(v);
            Vec3SoA sin(n);
            ref::AoSToSoA(v.data(), sin, n);

            auto aos = [&](const std::string& name, auto fn) {
                GuardedArray<Vec3f> out(n);
                RunGuarded(t + " | " + name, [&] { fn(gin.Data(), out.Data(), n); });
                return std::pair{ name, Flatten(out.ToVector()) };
            };
            auto inPlace = [&](const std::string& name, auto fn) {
                GuardedArray<Vec3f> io(v);
                RunGuarded(t + " | " + name, [&] { fn(io.Data(), io.Data(), n); });
                return std::pair{ name, Flatten(io.ToVector()) };
            };
            auto soa = [&](const std::string& name, auto fn) {
                Vec3SoA out = MakeSoAOutput(n);
                fn(sin, out, n);
                CheckSentinels(t + " | " + name, out.x, n);
                CheckSentinels(t + " | " + name, out.y, n);
                CheckSentinels(t + " | " + name, out.z, n);
                return std::pair{ name, Flatten(out, n) };
            };

            return { aos("ref  AoS", ref::NormalizeBatch), aos("simd AoS", simd::NormalizeBatch),
                     inPlace("simd AoS en place", simd::NormalizeBatch),
                     soa("ref  SoA", ref::NormalizeBatchSoA), soa("simd SoA", simd::NormalizeBatchSoA) };
        }

        Variants RunTransform(const std::string& t, const Mat4f& m, const std::vector<Vec3f>& p)
        {
            const std::size_t n = p.size();
            GuardedArray<Vec3f> gin(p);
            Vec3SoA sin(n);
            ref::AoSToSoA(p.data(), sin, n);

            auto aos = [&](const std::string& name, auto fn) {
                GuardedArray<Vec3f> out(n);
                RunGuarded(t + " | " + name, [&] { fn(m, gin.Data(), out.Data(), n); });
                return std::pair{ name, Flatten(out.ToVector()) };
            };
            auto inPlace = [&](const std::string& name, auto fn) {
                GuardedArray<Vec3f> io(p);
                RunGuarded(t + " | " + name, [&] { fn(m, io.Data(), io.Data(), n); });
                return std::pair{ name, Flatten(io.ToVector()) };
            };
            auto soa = [&](const std::string& name, auto fn) {
                Vec3SoA out = MakeSoAOutput(n);
                fn(m, sin, out, n);
                CheckSentinels(t + " | " + name, out.x, n);
                CheckSentinels(t + " | " + name, out.y, n);
                CheckSentinels(t + " | " + name, out.z, n);
                return std::pair{ name, Flatten(out, n) };
            };
            // Chaine complete quand les donnees sont en AoS : conversion -> calcul SoA -> conversion inverse.
            auto soaWithConversions = [&](const std::string& name) {
                const std::string key = t + " | " + name;
                Vec3SoA tmpIn = MakeSoAOutput(n), tmpOut = MakeSoAOutput(n);
                GuardedArray<Vec3f> out(n);
                RunGuarded(key, [&] { simd::AoSToSoA(gin.Data(), tmpIn, n); });
                simd::TransformPointsBatchSoA(m, tmpIn, tmpOut, n);
                RunGuarded(key, [&] { simd::SoAToAoS(tmpOut, out.Data(), n); });
                CheckSentinels(key, tmpIn.x, n);
                CheckSentinels(key, tmpOut.x, n);
                return std::pair{ name, Flatten(out.ToVector()) };
            };

            return { aos("ref  AoS", ref::TransformPointsBatch),
                     aos("simd AoS 4 pts/iter", simd::TransformPointsBatch),
                     aos("simd AoS 1 pt/iter", simd::TransformPointsBatchPerPoint),
                     inPlace("simd AoS 4 pts/iter en place", simd::TransformPointsBatch),
                     inPlace("simd AoS 1 pt/iter en place", simd::TransformPointsBatchPerPoint),
                     soa("ref  SoA", ref::TransformPointsBatchSoA),
                     soa("simd SoA", simd::TransformPointsBatchSoA),
                     soaWithConversions("simd SoA + conversions") };
        }

        Variants RunMultiply(const std::string& t, const std::vector<Mat4f>& a, const std::vector<Mat4f>& b)
        {
            const std::size_t n = a.size();
            GuardedArray<Mat4f> ga(a), gb(b);

            auto batch = [&](const std::string& name, auto fn) {
                GuardedArray<Mat4f> out(n);
                RunGuarded(t + " | " + name, [&] { fn(ga.Data(), gb.Data(), out.Data(), n); });
                return std::pair{ name, Flatten(out.ToVector()) };
            };
            auto inPlaceA = [&](const std::string& name) { // a[i] = a[i] * b[i]
                GuardedArray<Mat4f> io(a);
                RunGuarded(t + " | " + name, [&] { simd::MultiplyBatch(io.Data(), gb.Data(), io.Data(), n); });
                return std::pair{ name, Flatten(io.ToVector()) };
            };
            auto inPlaceB = [&](const std::string& name) { // b[i] = a[i] * b[i]
                GuardedArray<Mat4f> io(b);
                RunGuarded(t + " | " + name, [&] { simd::MultiplyBatch(ga.Data(), io.Data(), io.Data(), n); });
                return std::pair{ name, Flatten(io.ToVector()) };
            };
            auto single = [&](const std::string& name) {
                std::vector<Mat4f> out(n);
                for (std::size_t i = 0; i < n; ++i)
                    out[i] = simd::Multiply(a[i], b[i]);
                return std::pair{ name, Flatten(out) };
            };

            return { batch("ref  (operator*)", ref::MultiplyBatch), batch("simd batch", simd::MultiplyBatch),
                     single("simd Multiply()"), inPlaceA("simd en place (out = a)"), inPlaceB("simd en place (out = b)") };
        }

        Variants RunConversions(const std::string& t, const std::vector<Vec3f>& v)
        {
            const std::size_t n = v.size();
            GuardedArray<Vec3f> gin(v);

            auto roundTrip = [&](const std::string& name, auto toSoA, auto toAoS) {
                const std::string key = t + " | " + name;
                Vec3SoA s = MakeSoAOutput(n);
                GuardedArray<Vec3f> out(n);
                RunGuarded(key, [&] { toSoA(gin.Data(), s, n); });
                CheckSentinels(key, s.x, n);
                CheckSentinels(key, s.y, n);
                CheckSentinels(key, s.z, n);
                RunGuarded(key, [&] { toAoS(s, out.Data(), n); });
                return std::pair{ name, Flatten(out.ToVector()) };
            };

            // Le premier element sert de reference bit a bit : ici, l'entree elle-meme.
            return { std::pair{ std::string("entree"), Flatten(v) },
                     roundTrip("ref  AoS->SoA->AoS", ref::AoSToSoA, ref::SoAToAoS),
                     roundTrip("simd AoS->SoA->AoS", simd::AoSToSoA, simd::SoAToAoS) };
        }

        // =================================================================
        // Verifications
        // =================================================================

        // Donnees aleatoires : |obtenu - exact| <= borne, plus comptage des resultats identiques a "ref AoS".
        void CheckAgainstExact(const std::string& t, std::size_t n, const Variants& variants,
                               const std::vector<double>& exact, const std::vector<double>& bound)
        {
            const Flat& refAoS = variants.front().second;
            for (const auto& [name, got] : variants)
            {
                if (name == "entree")
                    continue;
                Stats& s = StatsFor(t + " | " + name);
                for (std::size_t i = 0; i < got.size(); ++i)
                {
                    const double err = std::fabs(static_cast<double>(got[i]) - exact[i]);
                    ++s.values;
                    if (!(err <= bound[i])) // faux aussi si got[i] est NaN
                    {
                        ++s.failures;
                        if (s.firstFailure.empty())
                        {
                            char buf[256];
                            std::snprintf(buf, sizeof(buf), "n=%zu, valeur %zu : obtenu %.9g, attendu %.9g (borne %.3g)",
                                          n, i, got[i], exact[i], bound[i]);
                            s.firstFailure = buf;
                        }
                    }
                    else if (bound[i] > 0.0)
                    {
                        s.worstRatio = std::max(s.worstRatio, err / bound[i]);
                    }

                    ++s.compared;
                    if (std::bit_cast<std::uint32_t>(got[i]) == std::bit_cast<std::uint32_t>(refAoS[i]))
                        ++s.identical;
                }
            }
        }

        // Resultats connus : entrees entieres exactes, valeurs attendues calculees a la main.
        // Tolerance 1e-6 absolue + 1e-6 relative : couvre l'arrondi de cos/sin(45 deg) en float
        // (ecart observe ~6e-8 sur une composante qui devrait valoir 0) et celui des divisions.
        void CheckKnown(const std::string& t, const Variants& variants, const Flat& expected)
        {
            for (const auto& [name, got] : variants)
            {
                Stats& s = StatsFor(t + " | " + name);
                for (std::size_t i = 0; i < got.size(); ++i)
                {
                    ++s.values;
                    const double diff = std::fabs(static_cast<double>(got[i]) - expected[i]);
                    if (!(diff <= 1e-6 + 1e-6 * std::fabs(expected[i])))
                    {
                        ++s.failures;
                        if (s.firstFailure.empty())
                        {
                            char buf[256];
                            std::snprintf(buf, sizeof(buf), "[resultat connu] valeur %zu : obtenu %.9g, attendu %.9g", i, got[i], expected[i]);
                            s.firstFailure = buf;
                        }
                    }
                }
            }
        }

        const std::string kDot = "Produit scalaire";
        const std::string kNorm = "Normalisation";
        const std::string kTransform = "Transformation";
        const std::string kMat = "Produit Mat4";
        const std::string kConv = "Conversions";

        void ValidateKnownResults()
        {
            // --- Produit scalaire : 5 elements = 1 bloc SIMD de 4 + 1 element de reste.
            const std::vector<Vec3f> a = { {1, 2, 3}, {1, 0, 0}, {0, 0, 0}, {-1, -2, -3}, {2, 0, 0} };
            const std::vector<Vec3f> b = { {4, 5, 6}, {0, 1, 0}, {5, 6, 7}, {1, 2, 3}, {3, 0, 0} };
            CheckKnown(kDot, RunDot(kDot, a, b), { 32, 0, 0, -14, 6 });

            // --- Normalisation : vecteur nul -> (0, 0, 0), NaN -> (0, 0, 0) (len > 0 est faux pour NaN).
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float s3 = 1.0f / std::sqrt(3.0f);
            const std::vector<Vec3f> v = { {3, 4, 0}, {0, 0, 0}, {0, 0, -5}, {1, 2, 2}, {1, 1, 1}, {nan, 0, 0} };
            CheckKnown(kNorm, RunNormalize(kNorm, v),
                       { 0.6f, 0.8f, 0, 0, 0, 0, 0, 0, -1, 1.0f / 3, 2.0f / 3, 2.0f / 3, s3, s3, s3, 0, 0, 0 });

            // --- Transformation de points (convention p' = p * M, w = 1).
            const std::vector<Vec3f> pts = { {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 2, 3} };
            const Vec3f t(1, 2, 3);
            auto rotZ90 = [](Vec3f p) { return Vec3f(-p.y, p.x, p.z); };
            auto expect = [&](auto f) { std::vector<Vec3f> e; for (const Vec3f& p : pts) e.push_back(f(p)); return Flatten(e); };

            const Mat4f I = Mat4f::Identity();
            const Mat4f T = Mat4f::Translate(t);
            const Mat4f R = Mat4f::Rotate(Quaternion::FromAxisAngle(Vec3f(0, 0, 1), 3.14159265f / 2));
            CheckKnown(kTransform, RunTransform(kTransform, I, pts), expect([](Vec3f p) { return p; }));
            CheckKnown(kTransform, RunTransform(kTransform, T, pts), expect([&](Vec3f p) { return p + t; }));
            CheckKnown(kTransform, RunTransform(kTransform, R, pts), expect(rotZ90));
            // Composition : T * R = translation PUIS rotation ; R * T = rotation PUIS translation.
            // Les deux produits sont calcules l'un avec la reference, l'autre avec le SIMD.
            CheckKnown(kTransform, RunTransform(kTransform, T * R, pts), expect([&](Vec3f p) { return rotZ90(p + t); }));
            CheckKnown(kTransform, RunTransform(kTransform, simd::Multiply(R, T), pts), expect([&](Vec3f p) { return rotZ90(p) + t; }));

            // --- Produit matriciel : A = 1..16 ; A * A connu, A * I = A, I * A = A.
            Mat4f A;
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    A.m[r][c] = static_cast<float>(4 * r + c + 1);
            const std::vector<Mat4f> lhs = { A, A, I };
            const std::vector<Mat4f> rhs = { A, I, A };
            Flat expected = { 90, 100, 110, 120, 202, 228, 254, 280, 314, 356, 398, 440, 426, 484, 542, 600 };
            const Flat flatA = Flatten(std::vector<Mat4f>{ A });
            expected.insert(expected.end(), flatA.begin(), flatA.end());
            expected.insert(expected.end(), flatA.begin(), flatA.end());
            CheckKnown(kMat, RunMultiply(kMat, lhs, rhs), expected);
        }

        void ValidateRandom(std::size_t n, Rng& rng)
        {
            // --- Produit scalaire (avec quelques vecteurs nuls)
            {
                std::vector<Vec3f> a = RandomVectors(n, rng), b = RandomVectors(n, rng);
                for (std::size_t i = 3; i < n; i += 7)
                    a[i] = Vec3f();
                std::vector<double> exact(n), bound(n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    const double px = double(a[i].x) * b[i].x, py = double(a[i].y) * b[i].y, pz = double(a[i].z) * b[i].z;
                    exact[i] = px + py + pz;
                    bound[i] = Gamma(3) * (std::fabs(px) + std::fabs(py) + std::fabs(pz));
                }
                CheckAgainstExact(kDot, n, RunDot(kDot, a, b), exact, bound);
            }

            // --- Normalisation (avec quelques vecteurs nuls).
            // Borne : somme des carres <= gamma(3), sqrt divise par 2 et ajoute u, division ajoute u
            //         => erreur relative par composante <= 3.5u (+ O(u^2)), on prend 4u.
            {
                std::vector<Vec3f> v = RandomVectors(n, rng);
                for (std::size_t i = 3; i < n; i += 7)
                    v[i] = Vec3f();
                std::vector<double> exact(3 * n), bound(3 * n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    const double x = v[i].x, y = v[i].y, z = v[i].z;
                    const double len = std::sqrt(x * x + y * y + z * z);
                    const double c[3] = { len > 0 ? x / len : 0.0, len > 0 ? y / len : 0.0, len > 0 ? z / len : 0.0 };
                    for (int k = 0; k < 3; ++k)
                    {
                        exact[3 * i + k] = c[k];
                        bound[3 * i + k] = 4.0 * kU * std::fabs(c[k]);
                    }
                }
                CheckAgainstExact(kNorm, n, RunNormalize(kNorm, v), exact, bound);
            }

            // --- Transformation par une matrice affine aleatoire (3 produits + 1 terme, 3 additions => gamma(4))
            {
                const Mat4f m = RandomAffine(rng);
                const std::vector<Vec3f> p = RandomVectors(n, rng);
                std::vector<double> exact(3 * n), bound(3 * n);
                for (std::size_t i = 0; i < n; ++i)
                    for (int c = 0; c < 3; ++c)
                    {
                        const double tx = double(p[i].x) * m.m[0][c], ty = double(p[i].y) * m.m[1][c];
                        const double tz = double(p[i].z) * m.m[2][c], tw = m.m[3][c];
                        exact[3 * i + c] = tx + ty + tz + tw;
                        bound[3 * i + c] = Gamma(4) * (std::fabs(tx) + std::fabs(ty) + std::fabs(tz) + std::fabs(tw));
                    }
                CheckAgainstExact(kTransform, n, RunTransform(kTransform, m, p), exact, bound);
            }

            // --- Produit matriciel (4 produits, 4 additions dans la reference : 0 + p0 + p1 + p2 + p3)
            {
                const std::vector<Mat4f> a = RandomMatrices(n, rng), b = RandomMatrices(n, rng);
                std::vector<double> exact(16 * n), bound(16 * n);
                for (std::size_t i = 0; i < n; ++i)
                    for (int r = 0; r < 4; ++r)
                        for (int c = 0; c < 4; ++c)
                        {
                            double sum = 0.0, abs = 0.0;
                            for (int k = 0; k < 4; ++k)
                            {
                                const double prod = double(a[i].m[r][k]) * b[i].m[k][c];
                                sum += prod;
                                abs += std::fabs(prod);
                            }
                            exact[16 * i + 4 * r + c] = sum;
                            bound[16 * i + 4 * r + c] = Gamma(4) * abs;
                        }
                CheckAgainstExact(kMat, n, RunMultiply(kMat, a, b), exact, bound);
            }

            // --- Conversions : simples copies, doivent etre exactes (borne = 0)
            {
                const std::vector<Vec3f> v = RandomVectors(n, rng);
                const Flat flat = Flatten(v);
                const std::vector<double> exact(flat.begin(), flat.end());
                const std::vector<double> bound(flat.size(), 0.0);
                CheckAgainstExact(kConv, n, RunConversions(kConv, v), exact, bound);
            }
        }
    }

    bool RunValidation(std::uint32_t seed)
    {
        g_stats.clear();
        Rng rng(seed);

        ValidateKnownResults();

        // Tailles 0..33 : lot vide, plus petit que la largeur SIMD, tous les restes possibles (n % 4).
        // Puis quelques grandes tailles non multiples de 4.
        std::vector<std::size_t> sizes;
        for (std::size_t n = 0; n <= 33; ++n)
            sizes.push_back(n);
        for (std::size_t n : { 1000, 1001, 1002, 1003, 4099 })
            sizes.push_back(n);
        for (std::size_t n : sizes)
            ValidateRandom(n, rng);

        std::printf("\n=== Validation (graine %u, tailles 0..33, 1000..1003, 4099) ===\n", seed);
        std::printf("  Borne = erreur d'arrondi theorique vs calcul en double ; 'ident.' = identique bit a bit a ref AoS\n\n");
        std::printf("  %-6s %-50s %10s %12s %20s\n", "", "traitement | version", "valeurs", "err/borne", "ident.");

        bool allOk = true;
        for (const auto& [key, s] : g_stats)
        {
            const bool ok = s.failures == 0;
            allOk = allOk && ok;
            char ident[32] = "-";
            if (s.compared > 0)
                std::snprintf(ident, sizeof(ident), "%zu/%zu", s.identical, s.compared);
            std::printf("  %-6s %-50s %10zu %11.1f%% %20s\n", ok ? "[OK]" : "[FAIL]", key.c_str(), s.values, 100.0 * s.worstRatio, ident);
            if (!ok)
                std::printf("         -> %zu echec(s), premier : %s\n", s.failures, s.firstFailure.c_str());
        }
        std::printf("\n  Resultat : %s\n", allOk ? "toutes les versions sont correctes" : "ECHEC, les mesures ne sont pas fiables");
        return allOk;
    }
}
