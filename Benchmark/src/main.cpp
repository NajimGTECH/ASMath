#include "Measure.h"
#include "SystemInfo.h"
#include "Workloads.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/*
 * Benchmark console application.
 *
 *   Benchmark.exe                       interactive menu (operation, version, batch size)
 *   Benchmark.exe --list                list the operations and their versions
 *   Benchmark.exe --op dot --version simd-aos --sizes 1000
 *   Benchmark.exe --suite               full campaign: every operation, every version, default sizes,
 *                                       raw results written to results/ (CSV)
 *
 * Options: --op <name>, --version <v1,v2|all>, --sizes <n1,n2>, --seed <n>, --samples <n>, --out <dir>
 */

using namespace bench;

namespace
{
    struct Options
    {
        std::string operation;             // empty = all operations (suite)
        std::vector<std::string> versions; // empty = all versions
        std::vector<std::size_t> sizes;    // empty = default sizes of the operation
        std::uint32_t seed = 42;
        MeasureSettings settings;
        std::string outDir;                // empty = no files written
        bool suite = false;
    };

    std::vector<std::string> SplitComma(const std::string& text)
    {
        std::vector<std::string> parts;
        std::stringstream stream(text);
        std::string part;
        while (std::getline(stream, part, ','))
            if (!part.empty())
                parts.push_back(part);
        return parts;
    }

    std::vector<std::size_t> ParseSizes(const std::string& text)
    {
        std::vector<std::size_t> sizes;
        for (const std::string& part : SplitComma(text))
            sizes.push_back(std::strtoull(part.c_str(), nullptr, 10));
        return sizes;
    }

    void PrintUsage()
    {
        std::printf(
            "Usage:\n"
            "  Benchmark.exe                      interactive menu\n"
            "  Benchmark.exe --list               list operations and versions\n"
            "  Benchmark.exe --suite              every operation and version, default sizes, CSV in results/\n"
            "  Benchmark.exe --op <name> [--version <v1,v2|all>] [--sizes <n1,n2,...>]\n"
            "Options:\n"
            "  --seed <n>      seed of the random input data (default 42)\n"
            "  --samples <n>   number of timed samples (default 31)\n"
            "  --out <dir>     write the raw results (CSV) to this folder\n");
    }

    void PrintList()
    {
        for (const Operation& op : AllOperations())
        {
            std::printf("%s : %s\n", op.name.c_str(), op.description.c_str());
            for (const Variant& v : op.variants)
                std::printf("    %-16s %s\n", v.name.c_str(), v.description.c_str());
        }
    }

    bool ParseArgs(int argc, char** argv, Options& opt)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            const bool hasValue = i + 1 < argc;
            if (arg == "--op" && hasValue)
                opt.operation = argv[++i];
            else if (arg == "--version" && hasValue)
            {
                const std::string value = argv[++i];
                if (value != "all")
                    opt.versions = SplitComma(value);
            }
            else if ((arg == "--sizes" || arg == "--size") && hasValue)
                opt.sizes = ParseSizes(argv[++i]);
            else if (arg == "--seed" && hasValue)
                opt.seed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            else if (arg == "--samples" && hasValue)
                opt.settings.samples = std::max(1, std::atoi(argv[++i]));
            else if (arg == "--out" && hasValue)
                opt.outDir = argv[++i];
            else if (arg == "--suite")
                opt.suite = true;
            else if (arg == "--list")
            {
                PrintList();
                std::exit(0);
            }
            else
            {
                PrintUsage();
                return false;
            }
        }
        return true;
    }

    // ====================================================================================
    // Interactive menu (used when the program is started without arguments, e.g. from VS)
    // ====================================================================================
    void InteractiveMenu(Options& opt)
    {
        const std::vector<Operation>& ops = AllOperations();
        std::printf("\nOperations:\n");
        for (std::size_t i = 0; i < ops.size(); ++i)
            std::printf("  %zu. %-10s %s\n", i + 1, ops[i].name.c_str(), ops[i].description.c_str());
        std::printf("  %zu. all (full campaign, results written to results/)\n", ops.size() + 1);
        std::printf("Choice: ");
        std::size_t choice = 0;
        std::cin >> choice;
        if (choice < 1 || choice > ops.size())
        {
            opt.suite = true;
            opt.outDir = "results";
            return;
        }
        const Operation& op = ops[choice - 1];
        opt.operation = op.name;

        std::printf("\nVersions of %s:\n", op.name.c_str());
        for (std::size_t i = 0; i < op.variants.size(); ++i)
            std::printf("  %zu. %-16s %s\n", i + 1, op.variants[i].name.c_str(), op.variants[i].description.c_str());
        std::printf("  %zu. all\n", op.variants.size() + 1);
        std::printf("Choice: ");
        std::cin >> choice;
        if (choice >= 1 && choice <= op.variants.size())
            opt.versions = { op.variants[choice - 1].name };

        std::printf("\nBatch sizes, comma separated (0 = default sizes): ");
        std::string sizes;
        std::cin >> sizes;
        opt.sizes = ParseSizes(sizes);
        if (opt.sizes.size() == 1 && opt.sizes[0] == 0)
            opt.sizes.clear();
    }

    // ====================================================================================
    // Result check: every version is compared with the reference on the same input
    // ====================================================================================
    std::string CheckAgainstReference(const std::vector<float>& reference, const std::vector<float>& values, bool exact, bool& ok)
    {
        double maxDiff = 0.0;
        std::size_t different = 0;
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (values[i] != reference[i])
            {
                ++different;
                maxDiff = std::max(maxDiff, std::fabs(static_cast<double>(values[i]) - reference[i]));
            }
        }

        char text[96];
        if (different == 0)
            std::snprintf(text, sizeof(text), "identical");
        else if (exact)
        {
            std::snprintf(text, sizeof(text), "MISMATCH (%zu values, max diff %.3g)", different, maxDiff);
            ok = false;
        }
        else
            std::snprintf(text, sizeof(text), "approx (max diff %.3g)", maxDiff);
        return text;
    }

    // ====================================================================================
    // Running and printing
    // ====================================================================================
    struct CsvFiles
    {
        std::ofstream summary;
        std::ofstream samples;
    };

    bool IsSelected(const Options& opt, const Variant& v)
    {
        if (opt.versions.empty())
            return true;
        for (const std::string& name : opt.versions)
            if (name == v.name)
                return true;
        return false;
    }

    bool RunOperation(const Operation& op, const Options& opt, CsvFiles* csv)
    {
        bool ok = true;
        const std::vector<std::size_t>& sizes = opt.sizes.empty() ? op.defaultSizes : opt.sizes;

        for (std::size_t n : sizes)
        {
            Workspace w;
            PrepareWorkspace(w, op, n, opt.seed);

            std::printf("\n=== %s, n = %zu : %s ===\n", op.name.c_str(), n, op.description.c_str());
            std::printf("%-16s %12s %10s %10s %7s %9s  %s\n", "version", "median", "q1", "q3", "IQR%", "speedup", "check");
            std::printf("%-16s %12s %10s %10s %7s %9s\n", "", "(ns/elem)", "", "", "", "(ref/v)");

            // The reference is always run (and measured) first: needed for the check and the ratio.
            const Variant& refVariant = op.variants.front();
            refVariant.run(w);
            const std::vector<float> reference = ReadOutput(w, refVariant.output);
            double referenceMedian = 0.0;

            for (std::size_t v = 0; v < op.variants.size(); ++v)
            {
                const Variant& variant = op.variants[v];
                const bool isReference = (v == 0);
                if (!isReference && !IsSelected(opt, variant))
                    continue;

                // Check the result (one run, outside the timing)
                variant.run(w);
                const std::vector<float> values = ReadOutput(w, variant.output);
                std::string check = "-";
                if (op.hasReference && isReference)
                    check = "reference";
                else if (op.hasReference && variant.output != Output::None)
                    check = CheckAgainstReference(reference, values, variant.exact, ok);

                // Measure
                const Measurement m = Measure(variant.run, w, opt.settings);
                if (isReference)
                    referenceMedian = m.median;
                const double speedup = (op.hasReference && m.median > 0.0) ? referenceMedian / m.median : 0.0;

                char speedupText[16] = "-";
                if (op.hasReference)
                    std::snprintf(speedupText, sizeof(speedupText), "%.2fx", speedup);
                std::printf("%-16s %12.3f %10.3f %10.3f %6.1f%% %9s  %s\n", variant.name.c_str(), m.median, m.q1, m.q3,
                            m.IqrPercent(), speedupText, check.c_str());

                if (csv != nullptr)
                {
                    csv->summary << op.name << ',' << n << ',' << variant.name << ',' << m.median << ',' << m.q1 << ','
                                 << m.q3 << ',' << m.min << ',' << m.max << ',' << m.IqrPercent() << ','
                                 << (op.hasReference ? speedup : 0.0) << ',' << m.callsPerSample << ",\"" << check << "\","
                                 << Checksum(values) << '\n';
                    for (std::size_t s = 0; s < m.samples.size(); ++s)
                        csv->samples << op.name << ',' << n << ',' << variant.name << ',' << s << ',' << m.samples[s] << '\n';
                }
            }
        }
        return ok;
    }

    std::string RerunCommand(const Options& opt)
    {
        std::string cmd = "Benchmark.exe";
        if (opt.suite)
            cmd += " --suite";
        if (!opt.operation.empty())
            cmd += " --op " + opt.operation;
        if (!opt.versions.empty())
        {
            cmd += " --version ";
            for (std::size_t i = 0; i < opt.versions.size(); ++i)
                cmd += (i ? "," : "") + opt.versions[i];
        }
        if (!opt.sizes.empty())
        {
            cmd += " --sizes ";
            for (std::size_t i = 0; i < opt.sizes.size(); ++i)
                cmd += (i ? "," : "") + std::to_string(opt.sizes[i]);
        }
        cmd += " --seed " + std::to_string(opt.seed) + " --samples " + std::to_string(opt.settings.samples);
        if (!opt.outDir.empty())
            cmd += " --out " + opt.outDir;
        return cmd;
    }
}

int main(int argc, char** argv)
{
    Options opt;
    if (!ParseArgs(argc, argv, opt))
        return 1;

    std::printf("=== ASM / SIMD math library benchmark: C++ reference vs SSE/SSE2 ===\n");
    const std::string report = SystemReport();
    std::printf("%s", report.c_str());

    if (argc == 1)
        InteractiveMenu(opt);
    if (opt.suite && opt.outDir.empty())
        opt.outDir = "results";

    const std::string rerun = RerunCommand(opt);
    std::printf("Seed %u, %d samples of >= %.1f ms after %.0f ms of warm-up per version\n", opt.seed,
                opt.settings.samples, opt.settings.minSampleMs, opt.settings.warmupMs);
    std::printf("Re-run with : %s\n", rerun.c_str());

    // Which operations?
    std::vector<const Operation*> selected;
    if (opt.operation.empty())
    {
        for (const Operation& op : AllOperations())
            selected.push_back(&op);
    }
    else
    {
        const Operation* op = FindOperation(opt.operation);
        if (op == nullptr)
        {
            std::printf("Unknown operation '%s'. Use --list.\n", opt.operation.c_str());
            return 1;
        }
        selected.push_back(op);
    }

    // Raw results files
    CsvFiles csv;
    CsvFiles* csvPtr = nullptr;
    if (!opt.outDir.empty())
    {
        std::filesystem::create_directories(opt.outDir);
        csv.summary.open(opt.outDir + "/summary.csv");
        csv.samples.open(opt.outDir + "/samples.csv");
        csv.summary << "operation,n,version,median_ns,q1_ns,q3_ns,min_ns,max_ns,iqr_percent,speedup_vs_ref,calls_per_sample,check,checksum\n";
        csv.samples << "operation,n,version,sample,ns_per_element\n";
        std::ofstream info(opt.outDir + "/run_info.txt");
        info << report << "Seed         : " << opt.seed << "\nSamples      : " << opt.settings.samples
             << "\nRe-run with  : " << rerun << "\n";
        csvPtr = &csv;
    }

    bool ok = true;
    for (const Operation* op : selected)
        ok = RunOperation(*op, opt, csvPtr) && ok;

    if (csvPtr != nullptr)
        std::printf("\nRaw results written to %s/ (summary.csv, samples.csv, run_info.txt)\n", opt.outDir.c_str());
    if (!ok)
        std::printf("\nERROR: at least one version gives a different result than the reference.\n");

    if (argc == 1)
    {
        std::printf("\nPress Enter to quit...");
        std::cin.ignore();
        std::cin.get();
    }
    return ok ? 0 : 1;
}
