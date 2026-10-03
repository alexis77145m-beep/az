#pragma once
#include "LSAFlangerCore.h"

namespace lsa
{
// One knob: 0 = perfectly clean (bit-exact dry), 1 = full metal flanger with slow L<->R pan.
inline Params macroToParams(float amount)
{
    const float a = std::clamp(amount, 0.0f, 1.0f);
    const float a2 = a * a * (3.0f - 2.0f * a); // smoothstep: gentle start, no jump
    Params p;
    p.rateHz = 0.22f; p.baseMs = 1.1f; p.tone = 0.7f; p.shape = 0.25f;
    p.stereo = 0.25f; p.panRateHz = 0.12f;
    p.mix      = 0.62f * a2;
    p.depth    = 0.45f + 0.45f * a;
    p.feedback = 0.80f * a2;
    p.drive    = 0.65f * a2;
    p.panDepth = 0.85f * a2;
    p.wetGain  = 1.0f / (1.0f + 0.9f * p.feedback * p.feedback * 4.0f * 0.25f * (1.0f + p.drive));
    p.outGain = std::pow(10.0f, (1.5f * std::sin(3.14159265f * a) + 0.7f * a2) / 20.0f); // makeup
    return p;
}
}
