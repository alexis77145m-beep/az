#include "../Source/MetalFlangerCore.h"
#include <cstdio>
int main()
{
    MetalFlangerCore c; c.prepare(48000);
    MetalFlangerCore::Params p; p.panRateHz = 0.5f; p.mix = 0.5f;
    double eL = 0, eR = 0; float peak = 0; bool ok = true;
    for (int i = 0; i < 48000 * 4; ++i) {
        float s = 0.3f * std::sin(6.2831853f * 220.f * i / 48000.f);
        float l = s, r = s; c.process(l, r, p);
        if (!std::isfinite(l) || !std::isfinite(r)) ok = false;
        peak = std::max(peak, std::max(std::fabs(l), std::fabs(r)));
        // energy per half-second window to show L/R movement
        eL += l * l; eR += r * r;
        if ((i + 1) % 12000 == 0) { std::printf("L %.3f  R %.3f\n", eL / 12000, eR / 12000); eL = eR = 0; }
    }
    std::printf("finite=%d peak=%.2f\n", ok, peak);
    return ok && peak < 4.f ? 0 : 1;
}
