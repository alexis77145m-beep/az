#pragma once
// LSA Flanger - DSP core (header-only, no JUCE dependency).
// Stereo flanger: cubic-Hermite modulated delay, anti-aliased (ADAA) saturation in the
// feedback loop, tone filter, smoothed parameters, and slow equal-power L<->R auto-pan.
#include <vector>
#include <cmath>
#include <algorithm>

namespace lsa
{
struct Params
{
    float rateHz    = 0.25f;  // flanger LFO speed
    float depth     = 0.7f;   // 0..1
    float baseMs    = 1.0f;   // centre delay (0.1..10 ms)
    float feedback  = 0.75f;  // -0.95..0.95 (negative = hollow/metallic)
    float drive     = 0.5f;   // 0..1 saturation in loop
    float tone      = 0.7f;   // 0..1 loop brightness
    float shape     = 0.0f;   // 0 = sine, 1 = triangle
    float stereo    = 0.25f;  // 0..0.5 R-channel LFO phase offset (cycles)
    float mix       = 0.5f;   // dry/wet
    float panRateHz = 0.15f;  // slow left<->right movement
    float panDepth  = 0.8f;   // 0..1
    float wetGain   = 1.0f;   // wet level compensation
    float outGain   = 1.0f;   // output makeup
};

class FlangerCore
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        size_t n = (size_t) (sr * 0.03) + 8; // 30 ms
        for (auto& d : delay) d.assign(n, 0.0f);
        reset();
        smoothCoef = 1.0f - std::exp(-1.0f / (0.02f * (float) sr)); // 20 ms
        first = true;
    }

    void reset()
    {
        wpos = 0; flangerPhase = 0.0; panPhase = 0.0;
        for (int c = 0; c < 2; ++c) { dcX[c] = dcY[c] = lp[c] = 0.0f; xPrev[c] = 0.0f; }
    }

    void process(float& l, float& r, const Params& target)
    {
        if (first) { cur = target; first = false; }
        smooth(target);
        const float dry[2] = { l, r };
        float wet[2];
        const double twoPi = 6.283185307179586;
        const int N = (int) delay[0].size();

        const float fbAmt = std::clamp(cur.feedback, -0.95f, 0.95f);
        const float g = 1.0f + cur.drive * 6.0f;
        const float lpCoef = 0.05f + 0.95f * cur.tone * cur.tone; // loop one-pole

        for (int ch = 0; ch < 2; ++ch)
        {
            double ph = flangerPhase + (ch ? (double) cur.stereo : 0.0);
            ph -= std::floor(ph);
            float s = 0.5f + 0.5f * (float) std::sin(twoPi * ph);
            float t = (float) (ph < 0.5 ? ph * 2.0 : 2.0 - ph * 2.0);
            float lfo = s + cur.shape * (t - s);

            float ms = cur.baseMs * (1.0f + cur.depth * 5.0f * lfo);
            float d = std::clamp(ms * 0.001f * (float) sr, 2.0f, (float) (N - 4));
            float rp = (float) wpos - d;
            if (rp < 0) rp += (float) N;
            int i1 = (int) rp; float f = rp - (float) i1;
            int i0 = (i1 - 1 + N) % N, i2 = (i1 + 1) % N, i3 = (i1 + 2) % N;
            const float* b = delay[ch].data();
            float y0 = b[i0], y1 = b[i1], y2 = b[i2], y3 = b[i3];
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            float y = ((c3 * f + c2) * f + c1) * f + y1;

            // tone (loop low-pass) then ADAA tanh saturation
            lp[ch] += lpCoef * (y - lp[ch]);
            float x = lp[ch] * g;
            float sat = adaaTanh(x, xPrev[ch]) / g;
            xPrev[ch] = x;

            float in = dry[ch] + fbAmt * sat * (1.0f + 0.5f * cur.drive);
            float hp = in - dcX[ch] + 0.995f * dcY[ch]; // DC blocker
            dcX[ch] = in; dcY[ch] = hp;
            delay[ch][wpos] = flush(hp);
            wet[ch] = y;
        }

        // dry/wet (equal-power)
        float wg = std::sin(cur.mix * 1.5707963f), dg = std::cos(cur.mix * 1.5707963f);
        float o0 = dry[0] * dg + wet[0] * wg * cur.wetGain, o1 = dry[1] * dg + wet[1] * wg * cur.wetGain;

        // slow equal-power auto-pan
        float pos = cur.panDepth * (float) std::sin(twoPi * panPhase);
        float a = (pos + 1.0f) * 0.78539816f;
        l = o0 * std::cos(a) * 1.41421356f * cur.outGain;
        r = o1 * std::sin(a) * 1.41421356f * cur.outGain;

        flangerPhase += cur.rateHz / sr; if (flangerPhase >= 1.0) flangerPhase -= 1.0;
        panPhase += cur.panRateHz / sr;  if (panPhase >= 1.0) panPhase -= 1.0;
        if (++wpos >= (size_t) N) wpos = 0;
    }

private:
    static float flush(float v) { return std::fabs(v) < 1e-20f ? 0.0f : v; }
    static float logcosh(float x)
    { float a = std::fabs(x); return a + std::log1p(std::exp(-2.0f * a)) - 0.693147181f; }
    // first-order antiderivative anti-aliasing of tanh
    static float adaaTanh(float x, float xp)
    {
        float dx = x - xp;
        if (std::fabs(dx) < 1e-4f) return std::tanh(0.5f * (x + xp));
        return (logcosh(x) - logcosh(xp)) / dx;
    }
    void smooth(const Params& t)
    {
        auto s = [this](float& c, float v) { c += smoothCoef * (v - c); };
        s(cur.rateHz, t.rateHz); s(cur.depth, t.depth); s(cur.baseMs, t.baseMs);
        s(cur.feedback, t.feedback); s(cur.drive, t.drive); s(cur.tone, t.tone);
        s(cur.shape, t.shape); s(cur.stereo, t.stereo); s(cur.mix, t.mix);
        s(cur.panRateHz, t.panRateHz); s(cur.panDepth, t.panDepth); s(cur.wetGain, t.wetGain); s(cur.outGain, t.outGain);
    }

    double sr = 44100.0, flangerPhase = 0.0, panPhase = 0.0;
    std::vector<float> delay[2];
    size_t wpos = 0;
    float dcX[2]{}, dcY[2]{}, lp[2]{}, xPrev[2]{};
    Params cur;
    float smoothCoef = 0.001f;
    bool first = true;
};
} // namespace lsa
