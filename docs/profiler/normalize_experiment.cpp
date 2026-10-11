// Experiment: does the temporary Vector3 created by Normalized() (ternary operator) cost time?
// Same compiler options as MathsLib (Release: /O2 /fp:precise, SSE2).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>
#include "Vector3.h"

using Vec3f = math::Vector3<float>;

__declspec(noinline) void WithNormalized(const Vec3f* in, Vec3f* out, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
        out[i] = in[i].Normalized();
}

__declspec(noinline) void WithoutTemporary(const Vec3f* in, Vec3f* out, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
    {
        const float len = in[i].Length();
        if (len > 0.0f)
        {
            out[i].x = in[i].x / len;
            out[i].y = in[i].y / len;
            out[i].z = in[i].z / len;
        }
        else
        {
            out[i].x = 0.0f;
            out[i].y = 0.0f;
            out[i].z = 0.0f;
        }
    }
}

template <typename F>
double Median(F f, const Vec3f* in, Vec3f* out, std::size_t n)
{
    std::vector<double> t;
    for (int s = 0; s < 101; ++s)
    {
        auto a = std::chrono::steady_clock::now();
        for (int r = 0; r < 20; ++r)
            f(in, out, n);
        auto b = std::chrono::steady_clock::now();
        t.push_back(std::chrono::duration<double, std::nano>(b - a).count() / (20.0 * n));
    }
    std::sort(t.begin(), t.end());
    return t[t.size() / 2];
}

int main()
{
    const std::size_t n = 100000;
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> d(-100.0f, 100.0f);
    std::vector<Vec3f> in(n), out1(n), out2(n);
    for (Vec3f& v : in)
        v = Vec3f(d(gen), d(gen), d(gen));

    Median(WithNormalized, in.data(), out1.data(), n); // warm-up
    const double t1 = Median(WithNormalized, in.data(), out1.data(), n);
    const double t2 = Median(WithoutTemporary, in.data(), out2.data(), n);

    int diff = 0;
    for (std::size_t i = 0; i < n; ++i)
        if (out1[i].x != out2[i].x || out1[i].y != out2[i].y || out1[i].z != out2[i].z)
            ++diff;

    std::printf("Normalized() (ternary + temporary) : %.3f ns/elem\n", t1);
    std::printf("explicit if, no temporary          : %.3f ns/elem\n", t2);
    std::printf("different results                  : %d\n", diff);
}
