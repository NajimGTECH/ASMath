#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "Data.h"
#include "Validation.h"
#include "Vector3Simd.h"
#include "Mat4Simd.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <intrin.h>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace nb = ankerl::nanobench;
using namespace math;

namespace
{
    // =====================================================================
    // Options de la ligne de commande
    // =====================================================================
    struct Options
    {
        std::string test = "all";           // dot | normalize | transform | mat4 | convert | all
        std::vector<std::size_t> sizes;     // vide = tailles par defaut
        std::uint32_t seed = 42;
        bool validateOnly = false;
        bool skipValidation = false;
        std::string outDir = "results";
    };

    // Tailles par defaut : petite (non multiple de 4), moyenne (tient en cache L1/L2), grande (sort des caches).
    const std::vector<std::size_t> kDefaultVecSizes = { 10, 1'000, 100'000, 4'000'000 };
    const std::vector<std::size_t> kDefaultMatSizes = { 10, 1'000, 100'000, 1'000'000 }; // 64 octets/matrice

    void PrintUsage()
    {
        std::printf(
            "Usage : Benchmark.exe [options]\n"
            "  --test <nom>      dot | normalize | transform | mat4 | convert | all (defaut : all)\n"
            "  --sizes a,b,c     tailles de lots (defaut : 10,1000,100000,4000000 ; mat4 : 10,1000,100000,1000000)\n"
            "  --seed <n>        graine des donnees aleatoires (defaut : 42)\n"
            "  --validate        validation uniquement, sans mesures\n"
            "  --no-validate     mesures sans validation prealable\n"
            "  --out <dossier>   dossier des resultats bruts (defaut : results)\n");
    }

    Options ParseArgs(int argc, char** argv)
    {
        Options opt;
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            const bool hasValue = i + 1 < argc;
            if (arg == "--test" && hasValue)
                opt.test = argv[++i];
            else if (arg == "--seed" && hasValue)
                opt.seed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            else if (arg == "--out" && hasValue)
                opt.outDir = argv[++i];
            else if (arg == "--sizes" && hasValue)
            {
                const std::string list = argv[++i];
                std::size_t start = 0;
                while (start < list.size())
                {
                    const std::size_t comma = list.find(',', start);
                    const std::string item = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                    const std::size_t n = std::strtoull(item.c_str(), nullptr, 10);
                    if (n > 0)
                        opt.sizes.push_back(n);
                    else
                        std::printf("Taille ignoree : '%s' (doit etre > 0 ; le lot vide est couvert par la validation)\n", item.c_str());
                    if (comma == std::string::npos)
                        break;
                    start = comma + 1;
                }
            }
            else if (arg == "--validate")
                opt.validateOnly = true;
            else if (arg == "--no-validate")
                opt.skipValidation = true;
            else
            {
                PrintUsage();
                std::exit(arg == "--help" || arg == "-h" ? 0 : 1);
            }
        }
        return opt;
    }

    // =====================================================================
    // Informations machine / compilation (necessaires pour reproduire les mesures)
    // =====================================================================
    std::string CpuBrand()
    {
        int regs[4] = {};
        __cpuid(regs, 0x80000000);
        if (static_cast<unsigned>(regs[0]) < 0x80000004u)
            return "inconnu";
        char brand[49] = {};
        for (int i = 0; i < 3; ++i)
        {
            __cpuid(regs, 0x80000002 + i);
            std::memcpy(brand + 16 * i, regs, 16);
        }
        std::string s(brand);
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
        return s;
    }

    std::string CpuFeatures()
    {
        int r0[4] = {}, r1[4] = {}, r7[4] = {};
        __cpuid(r0, 0);
        __cpuid(r1, 1);
        if (r0[0] >= 7)
            __cpuidex(r7, 7, 0);

        std::string s;
        auto add = [&](bool has, const char* name) { if (has) { s += name; s += ' '; } };
        add(r1[3] & (1 << 25), "SSE");
        add(r1[3] & (1 << 26), "SSE2");
        add(r1[2] & (1 << 0), "SSE3");
        add(r1[2] & (1 << 9), "SSSE3");
        add(r1[2] & (1 << 19), "SSE4.1");
        add(r1[2] & (1 << 20), "SSE4.2");
        add(r1[2] & (1 << 28), "AVX");
        add(r1[2] & (1 << 12), "FMA");
        add(r7[1] & (1 << 5), "AVX2");
        add(r7[1] & (1 << 16), "AVX-512F");
        return s;
    }

    const char* ArchFlag()
    {
#if defined(__AVX512F__)
        return "/arch:AVX512";
#elif defined(__AVX2__)
        return "/arch:AVX2";
#elif defined(__AVX__)
        return "/arch:AVX";
#else
        return "/arch:SSE2 (defaut x64, pas de FMA)";
#endif
    }

    const char* FpModel()
    {
#if defined(_M_FP_FAST)
        return "/fp:fast";
#elif defined(_M_FP_STRICT)
        return "/fp:strict";
#elif defined(_M_FP_PRECISE)
        return "/fp:precise";
#else
        return "inconnu";
#endif
    }

    void PrintSystemInfo(const Options& opt)
    {
        std::printf("=== Banc de mesure ASM/SIMD : reference C++ vs SSE/SSE2 (nanobench) ===\n");
        std::printf("  CPU            : %s\n", CpuBrand().c_str());
        std::printf("  Jeux d'instr.  : %s\n", CpuFeatures().c_str());
        std::printf("  Compilateur    : MSVC %d.%02d.%05d\n", _MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000);
#ifdef NDEBUG
        std::printf("  Configuration  : Release x64, /O2, /GL + /LTCG, auto-vectorisation autorisee\n");
#else
        std::printf("  Configuration  : DEBUG -> mesures non representatives, utiliser Release x64 !\n");
#endif
        std::printf("  Options        : %s, %s (identiques pour MathsLib et Benchmark)\n", ArchFlag(), FpModel());
        if (IsDebuggerPresent())
            std::printf("  ATTENTION      : debogueur attache -> lancer sans debogueur (Ctrl+F5) pour des mesures fiables\n");

        std::string sizes;
        for (std::size_t n : opt.sizes)
            sizes += (sizes.empty() ? "" : ",") + std::to_string(n);
        std::printf("  Relancer       : Benchmark.exe --test %s%s%s --seed %u\n",
                    opt.test.c_str(), sizes.empty() ? "" : " --sizes ", sizes.c_str(), opt.seed);
    }

    // =====================================================================
    // nanobench
    // =====================================================================

    /**
     * Protocole commun a toutes les mesures :
     *  - les donnees et buffers de sortie sont prepares AVANT bench.run (pas d'allocation chronometree) ;
     *  - les sorties sont ecrites dans des tableaux separes : les entrees ne sont jamais modifiees,
     *    il n'y a donc rien a reinitialiser entre deux iterations ;
     *  - warmup : iterations non mesurees (caches, predicteur de branchement, montee en frequence) ;
     *  - epochs : 21 mesures independantes -> nanobench donne la mediane et err% (MdAPE = ecart
     *    median a la mediane, robuste aux valeurs aberrantes) ;
     *  - minEpochTime : chaque mesure dure au moins 5 ms (petits lots : nanobench repete la boucle).
     */
    nb::Bench MakeBench(const std::string& title, std::size_t n)
    {
        nb::Bench b;
        b.title(title + " (n = " + std::to_string(n) + ")")
            .unit("elem")
            .batch(n)               // temps affiche par element (ns/elem)
            .relative(true)         // la 1re version (reference C++ AoS) vaut 100 %
            .warmup(10)
            .epochs(21)
            .minEpochTime(std::chrono::milliseconds(5))
            .performanceCounters(false); // compteurs materiels non disponibles sous Windows
        return b;
    }

    // Empeche le compilateur de supprimer le calcul : l'adresse du resultat "s'echappe" vers une
    // fonction opaque de nanobench (compilee sans optimisation), donc les ecritures doivent avoir lieu.
    template <typename T>
    void Escape(std::vector<T>& v) { nb::doNotOptimizeAway(v.data()); }
    void Escape(Vec3SoA& s) { Escape(s.x); Escape(s.y); Escape(s.z); }

    struct Row
    {
        std::string test;
        std::size_t n;
        std::string variant;
        double nsPerElem;
        double errPercent;
        double speedup; // temps reference / temps version
    };
    std::vector<Row> g_rows;

    void Record(const std::string& test, std::size_t n, const nb::Bench& b, const std::string& outDir)
    {
        using M = nb::Result::Measure;
        const auto& results = b.results();
        const double base = results.front().median(M::elapsed); // secondes par iteration (= lot complet)
        for (const auto& r : results)
        {
            const double t = r.median(M::elapsed);
            g_rows.push_back({ test, n, r.config().mBenchmarkName, t / static_cast<double>(n) * 1e9,
                               r.medianAbsolutePercentError(M::elapsed) * 100.0, base / t });
        }

        // Resultats bruts : toutes les mesures (epochs) au format JSON.
        std::ofstream json(outDir + "/" + test + "_n" + std::to_string(n) + ".json");
        nb::render(nb::templates::json(), b, json);
    }

    void BenchDot(std::size_t n, std::uint32_t seed, const std::string& outDir)
    {
        bench::Rng rng(seed);
        const std::vector<Vec3f> a = bench::RandomVectors(n, rng);
        const std::vector<Vec3f> c = bench::RandomVectors(n, rng);
        std::vector<float> out(n);
        Vec3SoA sa(n), sc(n), tmpA(n), tmpC(n);
        ref::AoSToSoA(a.data(), sa, n);
        ref::AoSToSoA(c.data(), sc, n);

        nb::Bench b = MakeBench("Produit scalaire", n);
        b.run("ref  AoS (C++)", [&] { ref::DotBatch(a.data(), c.data(), out.data(), n); Escape(out); });
        b.run("simd AoS (SSE)", [&] { simd::DotBatch(a.data(), c.data(), out.data(), n); Escape(out); });
        b.run("ref  SoA (C++)", [&] { ref::DotBatchSoA(sa, sc, out.data(), n); Escape(out); });
        b.run("simd SoA (SSE)", [&] { simd::DotBatchSoA(sa, sc, out.data(), n); Escape(out); });
        b.run("simd SoA + conversion AoS->SoA", [&] {
            simd::AoSToSoA(a.data(), tmpA, n);
            simd::AoSToSoA(c.data(), tmpC, n);
            simd::DotBatchSoA(tmpA, tmpC, out.data(), n);
            Escape(out);
        });
        Record("dot", n, b, outDir);
    }

    void BenchNormalize(std::size_t n, std::uint32_t seed, const std::string& outDir)
    {
        bench::Rng rng(seed);
        const std::vector<Vec3f> in = bench::RandomVectors(n, rng);
        std::vector<Vec3f> out(n);
        Vec3SoA sin(n), sout(n), tmpIn(n), tmpOut(n);
        ref::AoSToSoA(in.data(), sin, n);

        nb::Bench b = MakeBench("Normalisation", n);
        b.run("ref  AoS (C++)", [&] { ref::NormalizeBatch(in.data(), out.data(), n); Escape(out); });
        b.run("simd AoS (SSE)", [&] { simd::NormalizeBatch(in.data(), out.data(), n); Escape(out); });
        b.run("ref  SoA (C++)", [&] { ref::NormalizeBatchSoA(sin, sout, n); Escape(sout); });
        b.run("simd SoA (SSE)", [&] { simd::NormalizeBatchSoA(sin, sout, n); Escape(sout); });
        b.run("simd SoA + conversions AoS<->SoA", [&] {
            simd::AoSToSoA(in.data(), tmpIn, n);
            simd::NormalizeBatchSoA(tmpIn, tmpOut, n);
            simd::SoAToAoS(tmpOut, out.data(), n);
            Escape(out);
        });
        Record("normalize", n, b, outDir);
    }

    void BenchTransform(std::size_t n, std::uint32_t seed, const std::string& outDir)
    {
        bench::Rng rng(seed);
        const Mat4f m = bench::RandomAffine(rng);
        const std::vector<Vec3f> in = bench::RandomVectors(n, rng);
        std::vector<Vec3f> out(n);
        Vec3SoA sin(n), sout(n), tmpIn(n), tmpOut(n);
        ref::AoSToSoA(in.data(), sin, n);

        nb::Bench b = MakeBench("Transformation de points (Mat4 affine, w = 1)", n);
        b.run("ref  AoS (C++)", [&] { ref::TransformPointsBatch(m, in.data(), out.data(), n); Escape(out); });
        b.run("simd AoS 4 pts/iter (SSE)", [&] { simd::TransformPointsBatch(m, in.data(), out.data(), n); Escape(out); });
        b.run("simd AoS 1 pt/iter (SSE)", [&] { simd::TransformPointsBatchPerPoint(m, in.data(), out.data(), n); Escape(out); });
        b.run("ref  SoA (C++)", [&] { ref::TransformPointsBatchSoA(m, sin, sout, n); Escape(sout); });
        b.run("simd SoA (SSE)", [&] { simd::TransformPointsBatchSoA(m, sin, sout, n); Escape(sout); });
        b.run("simd SoA + conversions AoS<->SoA", [&] {
            simd::AoSToSoA(in.data(), tmpIn, n);
            simd::TransformPointsBatchSoA(m, tmpIn, tmpOut, n);
            simd::SoAToAoS(tmpOut, out.data(), n);
            Escape(out);
        });
        Record("transform", n, b, outDir);
    }

    void BenchMat4(std::size_t n, std::uint32_t seed, const std::string& outDir)
    {
        bench::Rng rng(seed);
        const std::vector<Mat4f> a = bench::RandomMatrices(n, rng);
        const std::vector<Mat4f> c = bench::RandomMatrices(n, rng);
        std::vector<Mat4f> out(n);

        nb::Bench b = MakeBench("Produit Mat4 x Mat4", n);
        b.run("ref  operator* (C++)", [&] { ref::MultiplyBatch(a.data(), c.data(), out.data(), n); Escape(out); });
        b.run("simd (SSE)", [&] { simd::MultiplyBatch(a.data(), c.data(), out.data(), n); Escape(out); });
        Record("mat4", n, b, outDir);
    }

    // Les conversions AoS <-> SoA sont mesurees a part (exigence du cahier des charges).
    void BenchConvert(std::size_t n, std::uint32_t seed, const std::string& outDir)
    {
        bench::Rng rng(seed);
        const std::vector<Vec3f> in = bench::RandomVectors(n, rng);
        std::vector<Vec3f> out(n);
        Vec3SoA soa(n);
        ref::AoSToSoA(in.data(), soa, n);

        nb::Bench toSoA = MakeBench("Conversion AoS -> SoA", n);
        toSoA.run("ref  AoS->SoA (C++)", [&] { ref::AoSToSoA(in.data(), soa, n); Escape(soa); });
        toSoA.run("simd AoS->SoA (SSE)", [&] { simd::AoSToSoA(in.data(), soa, n); Escape(soa); });
        Record("aos_to_soa", n, toSoA, outDir);

        nb::Bench toAoS = MakeBench("Conversion SoA -> AoS", n);
        toAoS.run("ref  SoA->AoS (C++)", [&] { ref::SoAToAoS(soa, out.data(), n); Escape(out); });
        toAoS.run("simd SoA->AoS (SSE)", [&] { simd::SoAToAoS(soa, out.data(), n); Escape(out); });
        Record("soa_to_aos", n, toAoS, outDir);
    }

    // =====================================================================
    // Synthese
    // =====================================================================
    void PrintSummary()
    {
        std::printf("\n=== Synthese : mediane par element, err%% = MdAPE (dispersion), acceleration = temps ref / temps version ===\n");
        std::printf("%-12s %10s  %-34s %12s %8s %14s\n", "traitement", "n", "version", "ns/elem", "err%", "acceleration");
        std::string previous;
        for (const Row& r : g_rows)
        {
            const std::string group = r.test + std::to_string(r.n);
            if (!previous.empty() && group != previous)
                std::printf("\n");
            previous = group;
            std::printf("%-12s %10zu  %-34s %12.3f %7.1f%% %13.2fx\n",
                        r.test.c_str(), r.n, r.variant.c_str(), r.nsPerElem, r.errPercent, r.speedup);
        }
    }

    void WriteSummaryCsv(const Options& opt)
    {
        const std::string path = opt.outDir + "/summary.csv";
        std::ofstream csv(path);
        csv << "test;n;version;ns_per_elem;err_percent;speedup_vs_ref;seed;cpu\n";
        for (const Row& r : g_rows)
            csv << r.test << ';' << r.n << ';' << r.variant << ';' << r.nsPerElem << ';' << r.errPercent << ';'
                << r.speedup << ';' << opt.seed << ';' << CpuBrand() << '\n';
        std::printf("\nResultats bruts : %s/*.json (toutes les mesures), synthese : %s\n", opt.outDir.c_str(), path.c_str());
    }
}

int main(int argc, char** argv)
{
    const Options opt = ParseArgs(argc, argv);
    PrintSystemInfo(opt);

    if (!opt.skipValidation && !bench::RunValidation(opt.seed))
        return 1; // inutile de mesurer des versions fausses
    if (opt.validateOnly)
        return 0;

    std::filesystem::create_directories(opt.outDir);

    struct Test
    {
        const char* name;
        void (*run)(std::size_t, std::uint32_t, const std::string&);
        const std::vector<std::size_t>& defaultSizes;
    };
    const Test tests[] = {
        { "dot", BenchDot, kDefaultVecSizes },
        { "normalize", BenchNormalize, kDefaultVecSizes },
        { "transform", BenchTransform, kDefaultVecSizes },
        { "mat4", BenchMat4, kDefaultMatSizes },
        { "convert", BenchConvert, kDefaultVecSizes },
    };

    bool found = false;
    for (const Test& t : tests)
    {
        if (opt.test != "all" && opt.test != t.name)
            continue;
        found = true;
        for (std::size_t n : (opt.sizes.empty() ? t.defaultSizes : opt.sizes))
            t.run(n, opt.seed, opt.outDir);
    }
    if (!found)
    {
        std::printf("Traitement inconnu : %s\n", opt.test.c_str());
        PrintUsage();
        return 1;
    }

    PrintSummary();
    WriteSummaryCsv(opt);
    return 0;
}
