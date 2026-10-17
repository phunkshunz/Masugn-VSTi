#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>
#include <vector>

// MASUGN: DSP-byggstenar (samma grund som VALV och KOLV). Allt här körs i ljudtråden: inga allokeringar.
namespace masugn
{
inline float fastTanh (float x) noexcept
{
    // Padé-approximation, mjuk och billig, klipps vid ±3
    x = juce::jlimit (-3.0f, 3.0f, x);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

//==============================================================================
// PolyBLEP: rundar av hoppen i vågformen så att det inte viker ner fula övertoner (aliasing).
// Oscillatorerna körs dessutom i 4x oversampling, så resultatet blir mycket rent.
inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)        { t /= dt;                 return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt;     return t * t + t + t + 1.0f; }
    return 0.0f;
}

struct Osc
{
    float phase = 0.0f;

    void reset (float p = 0.0f) noexcept { phase = p - std::floor (p); }

    // morph 0 = såg, 1 = fyrkant (mjuk övergång emellan)
    // skew 0..1: SKEV trycker ihop första halvan av vågen, så att den blir sned och nasal
    float next (float dt, float morph, float skew = 0.0f) noexcept
    {
        const float t = phase;
        float out;
        if (skew < 0.001f)
        {
            float saw = 2.0f * t - 1.0f - polyBlep (t, dt);
            float t2 = t + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;
            float sq = (t < 0.5f ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt);
            out = saw + morph * (sq - saw);
        }
        else
        {
            const float w = 0.5f - 0.45f * skew; // var mitten av vågen hamnar
            const float tw = t < w ? 0.5f * t / w : 0.5f + 0.5f * (t - w) / (1.0f - w);
            float saw = 2.0f * tw - 1.0f - polyBlep (t, dt);
            float t2 = t - w; if (t2 < 0.0f) t2 += 1.0f;
            float sq = (t < w ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt);
            out = saw + morph * (sq - saw);
        }
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f) phase += 1.0f;
        return out;
    }
};

//==============================================================================
// Zero-delay-feedback ladder-filter (Zavalishin). Stabilt vid snabb modulering,
// självsvänger snyggt och tanh i återkopplingen gör att det mättas analogt i stället för att skena.
// Koefficienterna sätts med set() en gång per kontrollsteg (16 sampel), inte varje sampel.
struct Ladder
{
    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    void set (float cutoffHz, float fs) noexcept
    {
        cutoffHz = juce::jlimit (20.0f, juce::jmin (20000.0f, fs * 0.42f), cutoffHz);
        const float g = std::tan (juce::MathConstants<float>::pi * cutoffHz / fs);
        G = g / (1.0f + g);
        b = 1.0f / (1.0f + g);
        G2 = G * G; G3 = G2 * G; G4 = G3 * G;
    }

    // k: 0..4 (över ca 4 självsvänger det). Ger 24 dB lågpass.
    float process (float x, float k, float drive) noexcept
    {
        const float S = G3 * b * s[0] + G2 * b * s[1] + G * b * s[2] + b * s[3];
        const float in = x * (1.0f + 0.35f * k);   // bas-kompensation
        float u = (in - k * S) / (1.0f + k * G4);
        u = fastTanh (u * drive) / drive;
        float v = u;
        for (int i = 0; i < 4; ++i)
        {
            const float vv = (v - s[i]) * G;
            const float y = vv + s[i];
            s[i] = y + vv;
            v = y;
        }
        return v;
    }

    float s[4] {};
    float G = 0.0f, b = 1.0f, G2 = 0.0f, G3 = 0.0f, G4 = 0.0f;
};

//==============================================================================
// SVF (Simper/Cytomic, trapets). Lågpass 12, band, högpass och notch ur samma filter.
struct Svf
{
    enum Out { LP, BP, HP, Notch };

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    // reso 0..1 (aldrig ända upp till självsvängning)
    void set (float cutoffHz, float reso, float fs) noexcept
    {
        cutoffHz = juce::jlimit (20.0f, juce::jmin (20000.0f, fs * 0.42f), cutoffHz);
        const float g = std::tan (juce::MathConstants<float>::pi * cutoffHz / fs);
        k = 2.0f - 1.94f * juce::jlimit (0.0f, 1.0f, reso);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float x, Out out) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        // mjuk mättnad i tillståndet, så att det aldrig kan skena
        ic1 = fastTanh (ic1 * 0.25f) * 4.0f;
        switch (out)
        {
            case LP:    return v2;
            case BP:    return v1 * k;            // normaliserad, ungefär samma nivå oavsett resonans
            case HP:    return x - k * v1 - v2;
            case Notch: return x - k * v1;
        }
        return v2;
    }

    float ic1 = 0.0f, ic2 = 0.0f, k = 2.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
};

//==============================================================================
// KAM: kamfilter med återkoppling, stämt efter cutoff. Metalliskt, plåtigt, som en sträng i ett rör.
struct Comb
{
    static constexpr int kSize = 8192;   // räcker ned till ca 24 Hz vid 192 kHz

    void init() { buf.assign ((size_t) kSize, 0.0f); pos = 0; damp = 0.0f; }   // i prepare, inte i ljudtråden
    void reset() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; damp = 0.0f; }

    void set (float hz, float reso, float fs) noexcept
    {
        delay = juce::jlimit (2.0f, (float) kSize - 4.0f, fs / juce::jmax (20.0f, hz));
        fb = 0.55f + 0.42f * juce::jlimit (0.0f, 1.0f, reso);
    }

    float process (float x) noexcept
    {
        float rp = (float) pos - delay;
        if (rp < 0.0f) rp += (float) kSize;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) & (kSize - 1);
        const float fr = rp - (float) i0;
        const float d = buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);
        damp += 0.45f * (d - damp);          // lite dämpning i återkopplingen, mjukare topp
        buf[(size_t) pos] = fastTanh (x + damp * fb);
        pos = (pos + 1) & (kSize - 1);
        return 0.5f * (x + d);
    }

    std::vector<float> buf;
    int pos = 0;
    float delay = 100.0f, fb = 0.5f, damp = 0.0f;
};

struct OnePole
{
    float z = 0.0f, a = 0.0f;
    void setCutoff (float hz, double sr) noexcept { a = std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sr); }
    float lp (float x) noexcept { z = x + a * (z - x); return z; }
    float hp (float x) noexcept { return x - lp (x); }
};

//==============================================================================
// SÄKERHETSVENTIL: limiter med kort förhandslyssning (lookahead) och ett mjukt tak.
// Hur hårt man än kör MASUGN kan den inte skena eller skära sönder öronen.
class SafetyValve
{
public:
    static constexpr float threshold = 0.71f; // ca -3 dB

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lookahead = juce::jmax (1, (int) std::round (sr * 0.0015));
        bufL.assign ((size_t) lookahead + 1, 0.0f);
        bufR.assign ((size_t) lookahead + 1, 0.0f);
        pos = 0;
        env = 0.0f;
        gain = 1.0f;
        attCoef = std::exp (-1.0f / (float) (sr * 0.0012));
        relCoef = std::exp (-1.0f / (float) (sr * 0.160));
    }

    int getLatency() const noexcept { return lookahead; }

    // stereo in/ut (länkad), returnerar största dämpningen i blocket (0..1, 0 = ingen)
    float process (float* L, float* R, int n) noexcept
    {
        float maxReduction = 0.0f;
        const int size = (int) bufL.size();
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            env = a > env ? a : env * relCoef + a * (1.0f - relCoef);
            const float want = env > threshold ? threshold / env : 1.0f;
            gain = want < gain ? want + (gain - want) * attCoef : want + (gain - want) * relCoef;

            bufL[(size_t) pos] = L[i];
            bufR[(size_t) pos] = R[i];
            int rp = pos - lookahead; if (rp < 0) rp += size;
            const float dl = bufL[(size_t) rp], dr = bufR[(size_t) rp];
            if (++pos >= size) pos = 0;

            L[i] = softCeiling (dl * gain);
            R[i] = softCeiling (dr * gain);
            maxReduction = juce::jmax (maxReduction, 1.0f - gain);
        }
        return maxReduction;
    }

    // rakt igenom upp till 0.8, sedan böjs det av mot 0.98. Sista skyddet.
    static float softCeiling (float x) noexcept
    {
        const float a = std::abs (x);
        if (a <= 0.8f) return x;
        return std::copysign (0.8f + 0.18f * std::tanh ((a - 0.8f) / 0.18f), x);
    }

private:
    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int pos = 0, lookahead = 1;
    float env = 0.0f, gain = 1.0f, attCoef = 0.0f, relCoef = 0.0f;
};
//==============================================================================
// VECK: wavefolder. Vågen viks tillbaka när den slår i taket: metalliskt, skevt, tjockt.
inline float fold (float x, float amount) noexcept
{
    if (amount < 0.001f) return x;
    const float g = 1.0f + 5.0f * amount;
    const float y = std::sin (x * g * juce::MathConstants<float>::halfPi);
    return x + amount * (y - x);
}

//==============================================================================
// EKO: ping-pong dub-delay med filter i återkopplingen och tejp-wobble.
class DubDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = (int) (sr * 4.0) + 8;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writePos = 0;
        for (auto& f : hp) { f.setCutoff (180.0f, sr); f.z = 0; }
        for (auto& f : lp) { f.setCutoff (3000.0f, sr); f.z = 0; }
        phase = 0;
        smoothedDelay.reset (sr, 0.25);
    }

    // in: mono sändning. ut: läggs till L/R. fb 0..0.95, tone 0..1, wobble 0..1
    void process (const float* in, float* outL, float* outR, int n, float delaySec, float fb, float tone, float wobble, float level)
    {
        const int size = (int) bufL.size();
        const float lpHz = 800.0f + tone * tone * 7000.0f;
        const float hpHz = 120.0f + tone * 400.0f;
        for (auto& f : lp) f.setCutoff (lpHz, sr);
        for (auto& f : hp) f.setCutoff (hpHz, sr);
        smoothedDelay.setTargetValue (juce::jlimit (0.01f, 3.5f, delaySec));
        const float feedback = juce::jlimit (0.0f, 0.95f, fb);
        const float lfoInc = juce::MathConstants<float>::twoPi * 0.55f / (float) sr;

        for (int i = 0; i < n; ++i)
        {
            phase += lfoInc;
            if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            const float wob = wobble * 0.004f * std::sin (phase) + wobble * 0.0015f * std::sin (phase * 2.7f);
            const float d = (smoothedDelay.getNextValue() + wob) * (float) sr;

            float rp = (float) writePos - d;
            while (rp < 0) rp += (float) size;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % size;
            const float fr = rp - (float) i0;
            const float dl = bufL[(size_t) i0] + fr * (bufL[(size_t) i1] - bufL[(size_t) i0]);
            const float dr = bufR[(size_t) i0] + fr * (bufR[(size_t) i1] - bufR[(size_t) i0]);

            outL[i] += dl * level;
            outR[i] += dr * level;

            // ping-pong: vänster matar höger och tvärtom
            const float fl = fastTanh (lp[0].lp (hp[0].hp (dr)) * feedback * 1.1f) / 1.1f;
            const float frr = fastTanh (lp[1].lp (hp[1].hp (dl)) * feedback * 1.1f) / 1.1f;
            bufL[(size_t) writePos] = in[i] + fl;
            bufR[(size_t) writePos] = frr;
            if (++writePos >= size) writePos = 0;
        }
    }

private:
    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int writePos = 0;
    OnePole hp[2], lp[2];
    float phase = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelay;
};

//==============================================================================
// KÖR: stereo-chorus med två svävande fördröjningar. Gör allt bredare och mjukare.
class Chorus
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = (int) (sr * 0.05) + 4;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        pos = 0; phase = 0.0f;
    }

    void process (float* L, float* R, int n, float mix) noexcept
    {
        const int size = (int) bufL.size();
        const float inc = juce::MathConstants<float>::twoPi * 0.37f / (float) sr;
        const float base = (float) sr * 0.009f, depth = (float) sr * 0.0032f;
        for (int i = 0; i < n; ++i)
        {
            phase += inc;
            if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            bufL[(size_t) pos] = L[i];
            bufR[(size_t) pos] = R[i];
            const float dl = base + depth * std::sin (phase);
            const float dr = base + depth * std::sin (phase + 2.1f);
            const float wl = read (bufL, size, dl), wr = read (bufR, size, dr);
            // korsat: vänster våt till höger och tvärtom, ger bredd
            L[i] = L[i] * (1.0f - 0.5f * mix) + wr * mix * 0.7f;
            R[i] = R[i] * (1.0f - 0.5f * mix) + wl * mix * 0.7f;
            if (++pos >= size) pos = 0;
        }
    }

private:
    float read (const std::vector<float>& b, int size, float d) const noexcept
    {
        float rp = (float) pos - d;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]);
    }

    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int pos = 0;
    float phase = 0.0f;
};
} // namespace masugn
