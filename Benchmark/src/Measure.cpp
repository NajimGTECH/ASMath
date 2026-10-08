#include "Measure.h"

#include <algorithm>
#include <chrono>

namespace bench
{
    namespace
    {
        using Clock = std::chrono::steady_clock; // QueryPerformanceCounter on Windows

        /** Calls the function `calls` times and returns the elapsed time in nanoseconds. */
        double TimeCalls(RunFunction run, Workspace& w, std::size_t calls)
        {
            // Volatile pointer: re-read before each call, the call cannot be optimized away.
            RunFunction volatile function = run;

            const Clock::time_point start = Clock::now();
            for (std::size_t i = 0; i < calls; ++i)
                function(w);
            const Clock::time_point end = Clock::now();

            return std::chrono::duration<double, std::nano>(end - start).count();
        }

        /** Value at a given fraction (0 = min, 0.5 = median, 1 = max) of a SORTED array, with linear interpolation. */
        double Percentile(const std::vector<double>& sorted, double fraction)
        {
            const double position = fraction * static_cast<double>(sorted.size() - 1);
            const std::size_t below = static_cast<std::size_t>(position);
            const std::size_t above = std::min(below + 1, sorted.size() - 1);
            const double t = position - static_cast<double>(below);
            return sorted[below] * (1.0 - t) + sorted[above] * t;
        }
    }

    Measurement Measure(RunFunction run, Workspace& w, const MeasureSettings& settings)
    {
        Measurement result;
        const double elements = static_cast<double>(std::max<std::size_t>(w.n, 1));

        // 1. Warm-up
        const double warmupNs = settings.warmupMs * 1e6;
        double warmedUp = 0.0;
        while (warmedUp < warmupNs)
            warmedUp += TimeCalls(run, w, 1);

        // 2. Calibration
        const double minSampleNs = settings.minSampleMs * 1e6;
        std::size_t calls = 1;
        while (TimeCalls(run, w, calls) < minSampleNs)
            calls *= 2;
        result.callsPerSample = calls;

        // 3. Samples
        for (int s = 0; s < settings.samples; ++s)
        {
            const double ns = TimeCalls(run, w, calls);
            result.samples.push_back(ns / (static_cast<double>(calls) * elements));
        }

        // 4. Statistics (median and quartiles are robust: a few slow samples caused by the OS do
        //    not change them, unlike the mean)
        std::vector<double> sorted = result.samples;
        std::sort(sorted.begin(), sorted.end());
        result.min = sorted.front();
        result.max = sorted.back();
        result.median = Percentile(sorted, 0.5);
        result.q1 = Percentile(sorted, 0.25);
        result.q3 = Percentile(sorted, 0.75);
        return result;
    }
}
