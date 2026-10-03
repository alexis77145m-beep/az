#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Metal flanger + slow auto-pan (left <-> right). Header-only, no JUCE dependency.
class MetalFlangerCore
{
public:
    struct Params
    {
        float rateHz     = 0.25f;  // flanger LFO speed
        float depth      = 0.7f;   // 0..1 sweep amount
        float baseMs     = 1.0f;   // centre delay (ms)
        float feedback   = 0.75f;  // -0.95..0.95 (negative = more metallic/hollow)
        float drive      = 0.5f;   // 0..1 saturation in feedback loop (metal grit)
        float mix        = 0.5f;   // dry/wet
        float panRateHz  = 0.15f;  // slow left<->right movement
        float panDepth   = 0.8f;   // 0..1
    };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        size_t n = (size_t) (sr * 0.02) + 4; // 20 ms max delay
        for (auto& d : delay) d.assign(n, 0.0f);
        wpos = 0; fbState[0] = fbState[1] = 0.0f;
        flangerPhase = 0.0; panPhase = 0.0;
    }

    void process(float& l, float& r, const Params& p)
    {
        const float dry[2] = { l, r };
        float wet[2];
        const double twoPi = 6.283185307179586;

        for (int ch = 0; ch < 2; ++ch)
        {
            // R channel LFO offset by 90 deg for stereo width
            double ph = flangerPhase + (ch ? 0.25 : 0.0);
            ph -= std::floor(ph);
            float lfo = 0.5f + 0.5f * (float) std::sin(twoPi * ph);
            float ms = p.baseMs + p.depth * 6.0f * lfo;
            float d = ms * 0.001f * (float) sr;
            float rp = (float) wpos - d;
            while (rp < 0) rp += (float) delay[ch].size();
            size_t i0 = (size_t) rp, i1 = (i0 + 1) % delay[ch].size();
            float fr = rp - (float) i0;
            float y = delay[ch][i0] * (1.0f - fr) + delay[ch][i1] * fr;

            // metal drive: tanh saturation in feedback path
            float g = 1.0f + p.drive * 8.0f;
            float fb = std::tanh(y * g) / g * (1.0f + p.drive);
            float in = dry[ch] + std::clamp(p.feedback, -0.95f, 0.95f) * fb;
            // DC block
            float hp = in - dcX[ch] + 0.995f * dcY[ch];
            dcX[ch] = in; dcY[ch] = hp;
            delay[ch][wpos] = hp;
            wet[ch] = y;
        }

        float o[2];
        for (int ch = 0; ch < 2; ++ch)
            o[ch] = dry[ch] * (1.0f - p.mix) + wet[ch] * p.mix;

        // slow equal-power auto-pan: pos in [-1,1]
        float pos = p.panDepth * (float) std::sin(twoPi * panPhase);
        float a = (pos + 1.0f) * 0.78539816f; // 0..pi/2
        float gl = std::cos(a) * 1.41421356f, gr = std::sin(a) * 1.41421356f;
        l = o[0] * gl;
        r = o[1] * gr;

        flangerPhase += p.rateHz / sr; if (flangerPhase >= 1.0) flangerPhase -= 1.0;
        panPhase     += p.panRateHz / sr; if (panPhase >= 1.0) panPhase -= 1.0;
        if (++wpos >= delay[0].size()) wpos = 0;
    }

private:
    double sr = 44100.0, flangerPhase = 0.0, panPhase = 0.0;
    std::vector<float> delay[2];
    size_t wpos = 0;
    float fbState[2] = {0, 0}, dcX[2] = {0, 0}, dcY[2] = {0, 0};
};
