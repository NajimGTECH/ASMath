#include "SystemInfo.h"

#include <cstring>
#include <intrin.h> // __cpuid

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // IsDebuggerPresent

namespace bench
{
    std::string CpuName()
    {
        // CPUID leaves 0x80000002..0x80000004 each return 16 characters of the brand string.
        int regs[4] = {};
        __cpuid(regs, 0x80000000);
        if (static_cast<unsigned>(regs[0]) < 0x80000004u)
            return "unknown";

        char name[49] = {};
        for (int i = 0; i < 3; ++i)
        {
            __cpuid(regs, 0x80000002 + i);
            std::memcpy(name + 16 * i, regs, 16);
        }

        std::string s(name);
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
        return s;
    }

    std::string CpuFeatures()
    {
        // CPUID leaf 1: ECX/EDX feature bits. Leaf 7: EBX extended features (AVX2).
        int leaf0[4] = {}, leaf1[4] = {}, leaf7[4] = {};
        __cpuid(leaf0, 0);
        __cpuid(leaf1, 1);
        if (leaf0[0] >= 7)
            __cpuidex(leaf7, 7, 0);

        const int ecx1 = leaf1[2], edx1 = leaf1[3], ebx7 = leaf7[1];
        std::string s;
        auto add = [&s](bool supported, const char* name)
        {
            if (supported)
                s += std::string(s.empty() ? "" : " ") + name;
        };
        add(edx1 & (1 << 25), "SSE");
        add(edx1 & (1 << 26), "SSE2");
        add(ecx1 & (1 << 0), "SSE3");
        add(ecx1 & (1 << 9), "SSSE3");
        add(ecx1 & (1 << 19), "SSE4.1");
        add(ecx1 & (1 << 20), "SSE4.2");
        add(ecx1 & (1 << 28), "AVX");
        add(ecx1 & (1 << 12), "FMA");
        add(ebx7 & (1 << 5), "AVX2");
        add(ebx7 & (1 << 16), "AVX-512F");
        return s;
    }

    std::string CompilerInfo()
    {
        // _MSC_FULL_VER = 193833145 -> "19.38.33145"
        std::string s = "MSVC " + std::to_string(_MSC_FULL_VER / 10000000) + "." +
                        std::to_string((_MSC_FULL_VER / 100000) % 100) + "." + std::to_string(_MSC_FULL_VER % 100000);

#ifdef NDEBUG
        s += ", Release x64, /O2";
#else
        s += ", DEBUG x64 (NOT representative)";
#endif

#if defined(__AVX2__)
        s += ", /arch:AVX2";
#elif defined(__AVX__)
        s += ", /arch:AVX";
#else
        s += ", default x64 instruction set (SSE2, no AVX, no FMA)";
#endif

#if defined(_M_FP_FAST)
        s += ", /fp:fast";
#elif defined(_M_FP_STRICT)
        s += ", /fp:strict";
#else
        s += ", /fp:precise";
#endif
        s += ", auto-vectorization allowed (default with /O2)";
        return s;
    }

    std::string SystemReport()
    {
        std::string s;
        s += "CPU          : " + CpuName() + "\n";
        s += "Instructions : " + CpuFeatures() + "\n";
        s += "Compiler     : " + CompilerInfo() + "\n";
        s += "Code used    : SSE/SSE2 intrinsics only (the AVX of the CPU is not used)\n";
#ifndef NDEBUG
        s += "WARNING      : Debug build, measurements are meaningless. Use Release x64.\n";
#endif
        if (IsDebuggerPresent())
            s += "WARNING      : a debugger is attached. Run without debugger (Ctrl+F5).\n";
        return s;
    }
}
