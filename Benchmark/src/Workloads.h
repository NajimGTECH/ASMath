#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "Vector3Batch.h"
#include "Mat4Batch.h"
#include "Vector2SSE.h"
#include "Mat3SSE.h"

/**
 * @file Workloads.h
 * @brief The processings ("operations") that can be measured, and their versions ("variants").
 */
namespace bench
{
    using Vec2f = math::Vector2<float>;
    using Mat3f = math::Mat3<float>;

    /**
     * @brief All the input and output buffers of one measurement.
     *
     * Everything is allocated and filled by PrepareWorkspace, BEFORE any timing: no allocation is
     * ever measured. The inputs are never modified by the measured functions (results go to
     * separate output buffers), so they do not need to be reset between two runs.
     */
    struct Workspace
    {
        std::size_t n = 0;
        math::Mat4f matrix;                 // transform: the same affine matrix for all points

        std::vector<math::Vec3f> a, b;      // AoS inputs
        math::Vec3SoA soaA, soaB;           // the same data in SoA layout (converted before timing)

        std::vector<float> outFloats;       // dot results
        std::vector<math::Vec3f> outVectors; // normalize / transform results (AoS)
        math::Vec3SoA outSoA;               // normalize / transform results (SoA)
        math::Vec3SoA tmpA, tmpB;           // buffers for the "with conversion" variants

        std::vector<math::Mat4f> matA, matB, outMatrices; // mat4mul extension only

        std::vector<Vec2f> v2A, v2B, outV2;                       // vec2 operations (AoS)
        math::Vector2SSE::Vector2SoA v2SoaA, v2SoaB, outV2SoA;    // vec2 operations (SoA)
        std::vector<Mat3f> m3A, m3B, outM3;                       // mat3 operations (AoS)
        math::Mat3SSE::Mat3SoA m3SoaA, m3SoaB, outM3SoA;          // mat3 operations (SoA)
        float outScalar = 0.0f;                                   // sum of the dot products / determinants
    };

    /** Which buffer a variant writes (used to check the results and compute a checksum). */
    enum class Output { Floats, Vectors, VectorsSoA, Matrices, Scalar, Vectors2, Vectors2SoA, Matrices3, Matrices3SoA, None };

    using RunFunction = void (*)(Workspace&);

    /** One version of an operation, e.g. "simd-aos" of "dot". */
    struct Variant
    {
        std::string name;
        std::string description;
        RunFunction run;
        Output output;
        bool exact; // true: must give the same values as the reference (false: approximation)
    };

    /** One processing, e.g. "dot". The first variant is the reference used for the speedups. */
    struct Operation
    {
        std::string name;
        std::string description;
        std::vector<Variant> variants;
        std::vector<std::size_t> defaultSizes;
        bool hasReference; // false for "convert": the speedup column is not meaningful
    };

    /** The list of all operations and variants. */
    const std::vector<Operation>& AllOperations();

    const Operation* FindOperation(const std::string& name);

    /** Allocates and fills every buffer for a batch of n elements (deterministic for a given seed). */
    void PrepareWorkspace(Workspace& w, const Operation& op, std::size_t n, std::uint32_t seed);

    /** Copies the output of a variant into a flat float array (SoA is re-interleaved as x, y, z / m00..m22). */
    std::vector<float> ReadOutput(const Workspace& w, Output output);

    /** Sum of all output values: printed with the results, so the computation is really "used". */
    double Checksum(const std::vector<float>& values);
}
