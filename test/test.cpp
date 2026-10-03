#include "../Source/LSAFlangerCore.h"
#include <cstdio>
int main()
{
    lsa::FlangerCore c; c.prepare(48000);
    lsa::Params p; p.panRateHz = 0.5f;
    double eL = 0, eR = 0; float peak = 0; bool ok = true;
    for (int i = 0; i < 48000 * 4; ++i) {
        float s = 0.3f * std::sin(6.2831853f * 220.f * i / 48000.f);
        float l = s, r = s; c.process(l, r, p);
        if (!std::isfinite(l) || !std::isfinite(r)) ok = false;
        peak = std::max(peak, std::max(std::fabs(l), std::fabs(r)));
        eL += l * l; eR += r * r;
        if ((i + 1) % 24000 == 0) { std::printf("L %.3f  R %.3f\n", eL / 24000, eR / 24000); eL = eR = 0; }
    }
    // extreme settings + silence tail must stay finite and decay
    p.feedback = 0.95f; p.drive = 1.f; p.depth = 1.f; p.mix = 1.f;
    for (int i = 0; i < 48000 * 2; ++i) { float l = i < 100 ? 1.f : 0.f, r = l; c.process(l, r, p);
        if (!std::isfinite(l) || !std::isfinite(r) || std::fabs(l) > 8.f) ok = false; }
    std::printf("finite/stable=%d peak=%.2f\n", ok, peak);
    return ok ? 0 : 1;
}
