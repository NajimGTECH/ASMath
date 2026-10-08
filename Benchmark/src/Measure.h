#pragma once
#include <cstddef>
#include <vector>
#include "Workloads.h"

/**
 * @file Measure.h
 * @brief Timing of one variant: warm-up, calibration, repeated samples and statistics.
 */
namespace bench
{
    struct MeasureSettings
    {
        int samples = 31;                  // number of timed samples (odd: the median is one real sample)
        double warmupMs = 50.0;            // warm-up duration before the samples
        double minSampleMs = 2.0;          // each sample lasts at least this long
    };

    /** Result of the measurement of one variant for one batch size. Times are in ns per element. */
    struct Measurement
    {
        std::vector<double> samples;       // raw values, in the order they were measured
        std::size_t callsPerSample = 0;    // how many times the batch function is called per sample
        double median = 0.0;
        double q1 = 0.0;                   // first quartile (25 %)
        double q3 = 0.0;                   // third quartile (75 %)
        double min = 0.0;
        double max = 0.0;

        /** Dispersion: interquartile range relative to the median, in %. */
        double IqrPercent() const { return median > 0.0 ? (q3 - q1) / median * 100.0 : 0.0; }
    };

    /**
     * @brief Measures `run(w)` (one call = one full batch of w.n elements).
     *
     * 1. warm-up: calls the function for `warmupMs` (caches, branch predictor, CPU frequency);
     * 2. calibration: doubles the number of calls until one sample lasts >= `minSampleMs`,
     *    so the timer resolution (100 ns for steady_clock on Windows) is negligible;
     * 3. `samples` timed samples; each value = sample duration / (calls * n) in ns per element.
     *
     * The function is called through a pointer read from a volatile variable: the compiler cannot
     * know which function is called, so it can neither inline it nor remove the calls.
     */
    Measurement Measure(RunFunction run, Workspace& w, const MeasureSettings& settings);
}
