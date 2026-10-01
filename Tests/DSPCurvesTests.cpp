#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "../Source/PluginProcessor.h"

using Catch::Matchers::WithinAbs;

TEST_CASE ("DSP Curves: Soft Clipping (tanh)", "[dsp][waveshaper]")
{
    SECTION ("Zero point")
    {
        REQUIRE_THAT (std::tanh (0.0f), WithinAbs (0.0f, 1e-6f));
    }

    SECTION ("Anti-symmetry f(-x) == -f(x)")
    {
        const float testValues[] = { 0.05f, 0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 50.0f };
        for (float x : testValues)
        {
            float pos = std::tanh (x);
            float neg = std::tanh (-x);
            REQUIRE_THAT (pos + neg, WithinAbs (0.0f, 1e-6f));
        }
    }

    SECTION ("Strict bounding in [-1.0, 1.0]")
    {
        const float extremeValues[] = { -1000.0f, -100.0f, -10.0f, 10.0f, 100.0f, 1000.0f };
        for (float x : extremeValues)
        {
            float y = std::tanh (x);
            REQUIRE (y >= -1.0f);
            REQUIRE (y <= 1.0f);
        }
    }

    SECTION ("Monotonic increase")
    {
        float prev = -1.0f;
        for (float x = -5.0f; x <= 5.0f; x += 0.25f)
        {
            float curr = std::tanh (x);
            REQUIRE (curr > prev);
            prev = curr;
        }
    }
}

TEST_CASE ("DSP Curves: Hard Clipping (clamp)", "[dsp][waveshaper]")
{
    auto clampFunc = [] (float x) { return juce::jlimit (-1.0f, 1.0f, x); };

    SECTION ("Linear region within [-1.0, 1.0]")
    {
        for (float x = -1.0f; x <= 1.0f; x += 0.1f)
        {
            REQUIRE_THAT (clampFunc (x), WithinAbs (x, 1e-6f));
        }
    }

    SECTION ("Saturation above +1.0")
    {
        const float highValues[] = { 1.0001f, 1.5f, 2.0f, 10.0f, 500.0f };
        for (float x : highValues)
        {
            REQUIRE_THAT (clampFunc (x), WithinAbs (1.0f, 1e-6f));
        }
    }

    SECTION ("Saturation below -1.0")
    {
        const float lowValues[] = { -1.0001f, -1.5f, -2.0f, -10.0f, -500.0f };
        for (float x : lowValues)
        {
            REQUIRE_THAT (clampFunc (x), WithinAbs (-1.0f, 1e-6f));
        }
    }
}
