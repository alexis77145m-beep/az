#include "../Source/LSAFlangerMacro.h"
#include <cstdio>
#include <random>
// Checks: amount 0 is bit-exact dry; level stays within +/-3 dB of dry across the knob; no NaN/blowup.
int main()
{
    std::mt19937 rng(1); std::normal_distribution<float> nd(0.f, 0.1f);
    bool ok = true;
    for (float a : { 0.f, 0.1f, 0.25f, 0.5f, 0.75f, 1.f })
    {
        lsa::FlangerCore c; c.prepare(48000);
        auto p = lsa::macroToParams(a);
        double ein = 0, eout = 0; float peak = 0; double maxdiff = 0;
        for (int i = 0; i < 48000 * 6; ++i)
        {
            float s = 0.2f * std::sin(6.2831853f * 196.f * i / 48000.f) + nd(rng) * 0.5f;
            float l = s, r = s; c.process(l, r, p);
            if (!std::isfinite(l) || !std::isfinite(r)) ok = false;
            if (i > 24000) { ein += 2.0 * s * s; eout += l * l + r * r; peak = std::max({peak, std::fabs(l), std::fabs(r)});
                             maxdiff = std::max(maxdiff, (double) std::fabs(l - s)); }
        }
        double db = 10 * std::log10(eout / ein);
        std::printf("amount %.2f: level %+.2f dB  peak %.2f  max|out-in| %.6f\n", a, db, peak, maxdiff);
        if (a == 0.f && maxdiff > 1e-6) ok = false;
        if (std::fabs(db) > 1.5 || peak > 1.5f) ok = false;
    }
    std::printf(ok ? "PASS\n" : "FAIL\n");
    return ok ? 0 : 1;
}
