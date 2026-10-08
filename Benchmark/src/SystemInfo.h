#pragma once
#include <string>

/**
 * @file SystemInfo.h
 * @brief Information needed to reproduce a measurement: CPU, instruction sets, compiler, options.
 */
namespace bench
{
    /** CPU name read with the CPUID instruction (e.g. "AMD Ryzen 7 9800X3D 8-Core Processor"). */
    std::string CpuName();

    /** Instruction sets supported by the CPU (SSE, SSE2, ..., AVX2, FMA). */
    std::string CpuFeatures();

    /** Compiler version and the options that change the generated code (/arch, /fp, Debug/Release). */
    std::string CompilerInfo();

    /** Multi-line text with all the above + warnings (Debug build, debugger attached). */
    std::string SystemReport();
}
