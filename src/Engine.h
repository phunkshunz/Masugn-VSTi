#pragma once

#include "Dsp.h"
#include "Tables.h"
#include <array>
#include <functional>

// MASUGN: ljudmotorn. Åtta röster med tre oscillatorer var. Varje oscillator kan vara
// vågtabell, partikel (granulär), resonator, kärna (FM) eller analog, och har en MUTATOR
// (FM, synk, veck, super, fasdiff) som förvränger vågen.
// BINAURAL: varje röst har två halvor, vänster och höger, med egna faser, filter och LFO-faser.
// Sedan två filter (serie eller parallellt) med ÖVERHETTNING (ljudet matas tillbaka in i filtret),
// tre kuvert, två LFO och en modulationsmatris.
// Rösterna räknas i 2x oversampling. Efter det: driv, kör, shimmer-rum, bredd och SÄKERHETSVENTILEN.
// Allt i ljudtråden är förallokerat.
namespace masugn
{
inline float mtof (float m) noexcept { return 440.0f * std::exp2 ((m - 69.0f) / 12.0f); }

enum OscType { TypeWave, TypeParticle, TypeResonator, TypeKernel, TypeAnalog, NumOscTypes };
enum Mutator { MutOff, MutFm, MutSync, MutFold, MutSuper, MutPhase, NumMutators };
enum FilterType { FLadder, FLp12, FBand, FHp, FNotch, FComb, FOff, NumFilterTypes };
enum PlayMode { Poly, Mono, Legato };

// Modulationsmatrisen: källor och mål
enum ModSrc { SrcOff, SrcLfo1, SrcLfo2, SrcEnv3, SrcFEnv, SrcVel, SrcWheel, SrcRandom, SrcKey, SrcAfter, NumSrc };
enum ModDst { DstOff, DstPos1, DstPos2, DstPos3, DstPosAll, DstForm1, DstForm2, DstForm3, DstLevel1, DstLevel2, DstLevel3,
              DstPitch, DstPitch2, DstPitch3, DstCut1, DstCut2, DstCutBoth, DstReso, DstNoise, DstVol, DstPan,
              DstMut1, DstMut2, DstMut3, NumDst };

constexpr int kModSlots = 4;
constexpr int kChunk = 16;   // kontrollsteg: modulering räknas om var 16:e sampel

//==============================================================================
struct OscP
{
    int type = TypeWave, table = 0, oct = 0, mut = MutOff;
    float pos = 0.0f, form = 0.0f, semi = 0.0f, fine = 0.0f, level = 0.0f, mamt = 0.0f;
};

struct ModSlot { int src = SrcOff, dst = DstOff; float amt = 0.0f; };

struct Params
{
    std::array<OscP, 3> osc {};
    float noise = 0.0f, spread = 0.3f, binaural = 0.0f;
    // FILTER
    int ftype1 = FLadder, ftype2 = FOff, froute = 0;
    float cut1 = 2000.0f, reso1 = 0.2f, cut2 = 2000.0f, reso2 = 0.2f, fenv = 0.4f, fkey = 0.3f, fdrive = 0.2f, heat = 0.0f;
    // KUVERT: filter, volym, kuvert 3 (a, d, s, r)
    std::array<std::array<float, 4>, 3> env {{ { 0.002f, 0.3f, 0.0f, 0.2f }, { 0.002f, 0.4f, 0.8f, 0.15f }, { 0.002f, 0.5f, 0.0f, 0.3f } }};
    // LFO
    std::array<int, 2> lfoWave {};
    std::array<float, 2> lfoHz { 2.0f, 0.5f };
    std::array<ModSlot, kModSlots> mod {};
    // SPEL
    int playMode = Poly; float glide = 0.0f;
    // EFFEKT
    float drive = 0.0f, chorus = 0.0f, shimmer = 0.0f, rumSize = 0.5f, rumMix = 0.0f, width = 0.0f, gain = 1.0f;
    // från MIDI
    float modWheel = 0.0f, bend = 0.0f; // bend i halvtoner
};

//==============================================================================
// Kuvert: linjär attack, exponentiell decay mot sustain och exponentiell release.
struct Env
{
    enum Stage { Idle, Attack, Decay, Release };
    Stage st = Idle;
    float v = 0.0f, aInc = 1.0f, dCoef = 0.0f, rCoef = 0.0f, sus = 0.0f;

    void set (const std::array<float, 4>& e, float fs) noexcept
    {
        aInc = 1.0f / (juce::jmax (0.0005f, e[0]) * fs);
        dCoef = std::exp (-3.0f / (juce::jmax (0.003f, e[1]) * fs));
        sus = juce::jlimit (0.0f, 1.0f, e[2]);
        rCoef = std::exp (-3.0f / (juce::jmax (0.004f, e[3]) * fs));
    }
    void gate (bool hardRestart) noexcept { if (hardRestart) v = 0.0f; st = Attack; }
    void release() noexcept { if (st != Idle) st = Release; }
    void kill() noexcept { st = Idle; v = 0.0f; }

    float next() noexcept
    {
        switch (st)
        {
            case Attack:  v += aInc; if (v >= 1.0f) { v = 1.0f; st = Decay; } break;
            case Decay:   v = sus + (v - sus) * dCoef; break;
            case Release: v *= rCoef; if (v < 0.0002f) kill(); break;
            case Idle:    break;
        }
        return v;
    }
};

//==============================================================================
// En oscillators tillstånd i en röst. Alla fem typerna har sitt tillstånd här,
// så att man kan byta typ mitt i en ton utan att något allokeras.
struct OscVoice
{
    float phase = 0.0f, mphase = 0.0f, mfb = 0.0f;
    Osc va;
    std::array<float, 7> superPh {};   // MUTATOR SUPER: sju svävande kopior

    // PARTIKEL
    struct Grain { bool on = false; float ph = 0.0f, inc = 0.0f, age = 0.0f, len = 1.0f; const float* data = nullptr; };
    std::array<Grain, 8> grains {};
    float spawn = 0.0f;

    // RESONATOR: åtta svängande moder
    static constexpr int kModes = 8;
    std::array<float, kModes> y1 {}, y2 {}, c1 {}, c2 {}, b {}, cont {};
    int nModes = 0, burst = 0, burstLen = 1;
    float exLp = 0.0f, contAmt = 0.0f;
    float resHz = -1.0f, resPos = -1.0f, resForm = -1.0f;   // senast uträknade, räknas bara om vid ändring

    void reset (float startPhase) noexcept
    {
        phase = startPhase; mphase = 0.0f; mfb = 0.0f;
        va.reset (startPhase);
        for (size_t k = 0; k < superPh.size(); ++k) superPh[k] = std::fmod (startPhase + 0.137f * (float) k * (float) k, 1.0f);
        for (auto& g : grains) g.on = false;
        spawn = 0.0f;
        y1.fill (0.0f); y2.fill (0.0f);
        exLp = 0.0f;
        resHz = -1.0f;
    }
};

inline float rand01 (juce::uint32& s) noexcept
{
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float) (s >> 8) * (1.0f / 16777216.0f);
}
inline float randBi (juce::uint32& s) noexcept { return rand01 (s) * 2.0f - 1.0f; }

// Resonatorns deltoner: sträng (harmonisk) -> stav/marimba -> klocka/platta
inline float modeRatio (int k, float material) noexcept
{
    static const float harm[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    static const float bar[8]  = { 1.0f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.81f, 31.87f };
    static const float bell[8] = { 1.0f, 1.594f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f };
    const float m = juce::jlimit (0.0f, 1.0f, material) * 2.0f;
    if (m < 1.0f) return harm[k] + (bar[k] - harm[k]) * m;
    return bar[k] + (bell[k] - bar[k]) * (m - 1.0f);
}

inline float kernelRatio (float form) noexcept
{
    static const float r[12] = { 0.5f, 1.0f, 1.41f, 2.0f, 3.0f, 3.5f, 4.0f, 5.0f, 6.0f, 7.0f, 9.0f, 11.0f };
    return r[juce::jlimit (0, 11, (int) std::round (form * 11.0f))];
}

// Renderar en oscillator ett kontrollsteg (n <= kChunk sampel) och LÄGGER TILL i out.
inline void renderOsc (OscVoice& s, int type, int table, float hz, float pos, float form, float level,
                       int mut, float mamt, float* out, int n, float fs, juce::uint32& rng) noexcept
{
    if (level < 0.0005f) return;
    const auto& bank = WaveBank::get();
    const float dt = juce::jlimit (0.0f, 0.45f, hz / fs);
    pos = juce::jlimit (0.0f, 1.0f, pos);
    form = juce::jlimit (0.0f, 1.0f, form);
    table = juce::jlimit (0, kNumTables - 1, table);
    mamt = juce::jlimit (0.0f, 1.0f, mamt);
    if (mamt < 0.001f) mut = MutOff;

    switch (type)
    {
        case TypeWave:
        {
            const int mip = WaveBank::mipFor (hz * (mut == MutSync ? 1.0f + mamt * 6.0f : 1.0f), fs);
            const float fpos = pos * (float) (kFrames - 1);
            const int f0 = juce::jmin (kFrames - 2, (int) fpos);
            const float fr = fpos - (float) f0;
            const float* a = bank.frame (table, mip, f0);
            const float* b = bank.frame (table, mip, f0 + 1);
            const float w = 0.5f - 0.45f * form;   // BÖJ: trycker ihop första halvan av vågen
            auto bend = [form, w] (float t) noexcept
            {
                return form > 0.001f ? (t < w ? 0.5f * t / w : 0.5f + 0.5f * (t - w) / (1.0f - w)) : t;
            };
            auto wrap = [] (float t) noexcept { return t - std::floor (t); };
            const float tp = juce::MathConstants<float>::twoPi;

            if (mut == MutSuper)
            {
                // SUPER: sju kopior av vågen, lite isär stämda, som en supersåg av vilken tabell som helst
                static const float det[7] = { -1.0f, -0.62f, -0.3f, 0.0f, 0.3f, 0.62f, 1.0f };
                static const float gain[7] = { 0.55f, 0.7f, 0.85f, 1.0f, 0.85f, 0.7f, 0.55f };
                const float spreadSt = mamt * 0.45f;   // upp till knappt en halv halvton isär
                std::array<float, 7> inc {};
                for (size_t k = 0; k < 7; ++k) inc[k] = juce::jmin (0.45f, dt * std::exp2 (det[k] * spreadSt / 12.0f));
                for (int i = 0; i < n; ++i)
                {
                    float sum = 0.0f;
                    for (size_t k = 0; k < 7; ++k)
                    {
                        sum += gain[k] * readFrames (a, b, fr, bend (s.superPh[k]));
                        s.superPh[k] += inc[k];
                        if (s.superPh[k] >= 1.0f) s.superPh[k] -= 1.0f;
                    }
                    out[i] += level * sum * 0.32f;
                    s.phase += dt; if (s.phase >= 1.0f) s.phase -= 1.0f;
                }
                break;
            }

            for (int i = 0; i < n; ++i)
            {
                const float t = s.phase;
                float x;
                switch (mut)
                {
                    case MutFm:     // FM: vågen vrider sin egen fas, metalliskt och skarpt
                        x = readFrames (a, b, fr, bend (wrap (t + mamt * 0.32f * std::sin (tp * t))));
                        break;
                    case MutSync:   // SYNK: hårdsynk, vågen startar om flera gånger per period
                        x = readFrames (a, b, fr, bend (wrap (t * (1.0f + mamt * 6.0f))));
                        break;
                    case MutPhase:  // FASDIFF: vågen minus en förskjuten kopia, som pulsbredd av vilken våg som helst
                        x = 0.7f * (readFrames (a, b, fr, bend (t)) - readFrames (a, b, fr, bend (wrap (t + 0.5f - 0.45f * mamt))));
                        break;
                    default:
                        x = readFrames (a, b, fr, bend (t));
                        break;
                }
                if (mut == MutFold) x = fold (x, mamt);   // VECK
                out[i] += level * x;
                s.phase += dt;
                if (s.phase >= 1.0f) s.phase -= 1.0f;
            }
            break;
        }

        case TypeParticle:
        {
            // TÄTHET: lite = glesa, långa korn. Mycket = täta, korta korn som sprakar och svävar.
            const float interval = (0.11f + (0.012f - 0.11f) * form) * fs;
            const float len = (0.16f + (0.035f - 0.16f) * form) * fs;
            const float cents = 4.0f + 40.0f * form;
            const float spray = 0.08f + 0.25f * form;
            const float norm = 1.1f / std::sqrt (juce::jmax (1.0f, len / interval));
            for (int i = 0; i < n; ++i)
            {
                s.spawn -= 1.0f;
                if (s.spawn <= 0.0f)
                {
                    s.spawn = interval * (0.6f + 0.8f * rand01 (rng));
                    for (auto& g : s.grains)
                        if (! g.on)
                        {
                            const float ghz = hz * std::exp2 (randBi (rng) * cents / 1200.0f);
                            const int fr = juce::jlimit (0, kFrames - 1, (int) std::round ((pos + randBi (rng) * spray) * (float) (kFrames - 1)));
                            g.on = true; g.age = 0.0f; g.len = len * (0.8f + 0.4f * rand01 (rng));
                            g.ph = rand01 (rng); g.inc = juce::jmin (0.45f, ghz / fs);
                            g.data = bank.frame (table, WaveBank::mipFor (ghz, fs), fr);
                            break;
                        }
                }
                float sum = 0.0f;
                for (auto& g : s.grains)
                {
                    if (! g.on) continue;
                    const float u = g.age / g.len;
                    const float win = 4.0f * u * (1.0f - u);
                    const float idx = g.ph * (float) kTableSize;
                    const int i0 = juce::jmin (kTableSize - 1, (int) idx);
                    const float v = g.data[i0] + (idx - (float) i0) * (g.data[i0 + 1] - g.data[i0]);
                    sum += v * win;
                    g.ph += g.inc; if (g.ph >= 1.0f) g.ph -= 1.0f;
                    g.age += 1.0f;
                    if (g.age >= g.len) g.on = false;
                }
                out[i] += level * sum * norm;
            }
            break;
        }

        case TypeResonator:
        {
            // MATERIAL (pos): sträng -> stav -> klocka. KLANG (form): hur länge det ringer.
            // räkna bara om deltonerna när tonhöjd, material eller klang faktiskt ändrats
            if (std::abs (hz - s.resHz) > hz * 0.0006f || std::abs (pos - s.resPos) > 0.002f || std::abs (form - s.resForm) > 0.002f)
            {
                s.resHz = hz; s.resPos = pos; s.resForm = form;
                static const float partGain[8] = { 1.0f, 0.6598f, 0.5170f, 0.4353f, 0.3807f, 0.3410f, 0.3105f, 0.2872f };   // 1 / (k+1)^0.6
                const float T = 0.06f * std::pow (70.0f, form);
                s.nModes = 0;
                for (int k = 0; k < OscVoice::kModes; ++k)
                {
                    const float f = hz * modeRatio (k, pos);
                    if (f > fs * 0.45f) break;
                    const float w = juce::MathConstants<float>::twoPi * f / fs;
                    const float Tk = T / (1.0f + 0.35f * (float) k);
                    const float r = std::exp (-6.9f / (Tk * fs));
                    s.c1[(size_t) k] = 2.0f * r * std::cos (w);
                    s.c2[(size_t) k] = -r * r;
                    s.b[(size_t) k] = std::sin (w) * partGain[k];
                    s.cont[(size_t) k] = std::sqrt (2.0f * (1.0f - r));
                    s.nModes = k + 1;
                }
            }
            const float contAmt = form * form * form * 0.35f;   // vid lång klang: lite stråke som håller tonen vid liv
            for (int i = 0; i < n; ++i)
            {
                const float nz = randBi (rng);
                float ex = 0.0f;
                if (s.burst > 0)
                {
                    ex = nz * (float) s.burst / (float) s.burstLen * 6.0f;
                    --s.burst;
                }
                s.exLp += 0.35f * (ex - s.exLp);
                float sum = 0.0f;
                for (int k = 0; k < s.nModes; ++k)
                {
                    const float in = s.exLp + contAmt * nz * s.cont[(size_t) k];
                    const float y = s.b[(size_t) k] * in + s.c1[(size_t) k] * s.y1[(size_t) k] + s.c2[(size_t) k] * s.y2[(size_t) k];
                    s.y2[(size_t) k] = s.y1[(size_t) k];
                    s.y1[(size_t) k] = y;
                    sum += y;
                }
                out[i] += level * fastTanh (sum * 0.6f);
            }
            break;
        }

        case TypeKernel:
        {
            // INDEX (pos): hur mycket modulatorn vrider. RATIO (form): modulatorns tonhöjd.
            const float mhz = hz * kernelRatio (form);
            const float tame = juce::jlimit (0.15f, 1.0f, 3000.0f / juce::jmax (1.0f, mhz)); // höga toner får mindre index
            const float index = pos * pos * 7.0f * tame;
            const float mdt = juce::jlimit (0.0f, 0.45f, mhz / fs);
            const float tp = juce::MathConstants<float>::twoPi;
            for (int i = 0; i < n; ++i)
            {
                const float m = std::sin (tp * s.mphase + s.mfb * 0.25f * pos);
                s.mfb = m;
                out[i] += level * std::sin (tp * s.phase + index * m);
                s.phase += dt;  if (s.phase >= 1.0f) s.phase -= 1.0f;
                s.mphase += mdt; if (s.mphase >= 1.0f) s.mphase -= 1.0f;
            }
            break;
        }

        case TypeAnalog:
        default:
            for (int i = 0; i < n; ++i)
                out[i] += level * 0.8f * (mut == MutFold ? fold (s.va.next (dt, pos, form), mamt) : s.va.next (dt, pos, form));
            break;
    }
}

//==============================================================================
// En halva av en röst: tre oscillatorer och två filter. BINAURAL ger varje röst två halvor,
// en för vänster och en för höger öra.
struct Half
{
    std::array<OscVoice, 3> osc {};
    Ladder lad[2];
    Svf svf[2];
    Comb comb[2];
    OnePole fbHp;        // ÖVERHETTNING: återkopplingen utan likström
    float last = 0.0f;
    juce::uint32 rng = 1;

    void reset (float phaseOffset) noexcept
    {
        for (int o = 0; o < 3; ++o) osc[(size_t) o].reset (std::fmod ((float) o * 0.31f + phaseOffset, 1.0f));
        for (int f = 0; f < 2; ++f) { lad[f].reset(); svf[f].reset(); comb[f].reset(); }
        last = 0.0f; fbHp.z = 0.0f;
    }

    float runFilter (int which, int type, float x, float k, float drive) noexcept
    {
        switch (type)
        {
            case FLadder: return lad[which].process (x, k, drive);
            case FLp12:   return svf[which].process (x, Svf::LP);
            case FBand:   return svf[which].process (x, Svf::BP);
            case FHp:     return svf[which].process (x, Svf::HP);
            case FNotch:  return svf[which].process (x, Svf::Notch);
            case FComb:   return comb[which].process (x);
            default:      return x;
        }
    }
};

struct Voice
{
    std::array<Half, 2> half {};
    Env fenv, aenv, menv;
    int note = -1;
    float vel = 1.0f, pitch = 60.0f, target = 60.0f, detune = 0.0f, basePan = 0.0f, rnd = 0.0f, fs = 96000.0f;
    bool held = false, sustained = false;
    juce::uint32 age = 0, rng = 1;

    bool active() const noexcept { return aenv.st != Env::Idle; }

    void start (int n, float velocity, float fromPitch, bool retrigger, juce::uint32 stamp, float binaural) noexcept
    {
        note = n; target = (float) n;
        pitch = fromPitch >= 0.0f ? fromPitch : (float) n;
        vel = velocity;
        held = true; sustained = false; age = stamp;
        rnd = randBi (rng);
        if (retrigger || ! active())
        {
            const bool wasSilent = ! active();
            if (wasSilent)
            {
                half[0].reset (0.0f);
                half[1].reset (binaural * (0.23f + 0.2f * rand01 (rng)));   // höger halva startar på en annan fas
            }
            for (auto& h : half)
                for (auto& o : h.osc)
                {
                    o.burstLen = juce::jmax (1, (int) (fs * (0.003f + 0.004f * (1.0f - vel))));
                    o.burst = o.burstLen;
                }
            fenv.gate (true);
            menv.gate (true);
            aenv.gate (wasSilent);
        }
    }

    void glideTo (int n) noexcept { note = n; target = (float) n; held = true; }
    void release() noexcept { held = false; fenv.release(); aenv.release(); menv.release(); }

    // Lägger till i L/R (oversamplad takt). lfo[l][side]: LFO l för vänster (0) och höger (1) halva.
    void render (float* L, float* R, int start, int end, const Params& p, const float* const (&lfo)[2][2],
                 float aftertouch) noexcept
    {
        const float velAmp = (0.55f + 0.45f * vel) * (p.playMode == Poly ? 0.55f : 1.0f);
        const float envOct = p.fenv * 7.0f * (0.75f + 0.25f * vel);
        const float drive = 1.0f + p.fdrive * 4.0f;
        const bool twoHalves = p.binaural > 0.001f;
        const float heat = juce::jlimit (0.0f, 1.0f, p.heat);
        const float heatGain = 1.0f + 2.5f * heat, heatComp = 1.0f / (1.0f + 1.2f * heat);
        float buf[2][kChunk];
        float amp[kChunk];

        for (int i = start; i < end; i += kChunk)
        {
            if (! active()) break;
            const int n = juce::jmin (kChunk, end - i);

            // GLID
            const float gc = p.glide > 0.001f ? 1.0f - std::exp (-3.0f * (float) n / (p.glide * fs)) : 1.0f;
            pitch += (target - pitch) * gc;

            // kuverten räknas en gång, delas av båda halvorna
            const float fe = fenv.v, me = menv.v;
            for (int j = 0; j < n; ++j)
            {
                fenv.next(); menv.next();
                amp[j] = aenv.next() * velAmp;
            }

            float volMod = 1.0f, panMod = 0.0f;
            for (int side = 0; side < (twoHalves ? 2 : 1); ++side)
            {
                auto& h = half[(size_t) side];
                const float sideSign = side == 0 ? -1.0f : 1.0f;
                const float bin = twoHalves ? p.binaural * sideSign : 0.0f;

                // MODULATION: källornas värden just nu (varje halva har sin egen LFO-fas)
                float src[NumSrc] {};
                src[SrcLfo1] = lfo[0][side][i]; src[SrcLfo2] = lfo[1][side][i];
                src[SrcEnv3] = me; src[SrcFEnv] = fe; src[SrcVel] = vel;
                src[SrcWheel] = p.modWheel; src[SrcRandom] = rnd;
                src[SrcKey] = juce::jlimit (-1.0f, 1.0f, ((float) note - 60.0f) / 24.0f);
                src[SrcAfter] = aftertouch;
                float d[NumDst] {};
                for (const auto& m : p.mod)
                    if (m.src > SrcOff && m.src < NumSrc && m.dst > DstOff && m.dst < NumDst)
                        d[m.dst] += m.amt * src[m.src];
                if (side == 0) { volMod = juce::jlimit (0.0f, 2.0f, 1.0f + d[DstVol]); panMod = d[DstPan]; }

                // binaural: halvorna stäms isär några cent och filtren glider isär
                const float basePitch = pitch + p.bend + d[DstPitch] * 12.0f + bin * 0.09f;

                float* bb = buf[side];
                std::fill (bb, bb + n, 0.0f);
                for (int o = 0; o < 3; ++o)
                {
                    const auto& op = p.osc[(size_t) o];
                    const float level = juce::jlimit (0.0f, 1.0f, op.level + d[DstLevel1 + o]);
                    if (level < 0.0005f) continue;
                    const float extra = o == 1 ? d[DstPitch2] * 12.0f : (o == 2 ? d[DstPitch3] * 12.0f : 0.0f);
                    const float pm = basePitch + 12.0f * (float) op.oct + op.semi + op.fine * 0.01f + extra
                                   + detune * p.spread * 0.08f * (float) (o - 1);
                    const float pos = op.pos + d[DstPos1 + o] + d[DstPosAll] + bin * 0.04f;
                    const float form = op.form + d[DstForm1 + o];
                    renderOsc (h.osc[(size_t) o], op.type, op.table, mtof (pm), pos, form, level * 0.6f,
                               op.mut, op.mamt + d[DstMut1 + o], bb, n, fs, h.rng);
                }

                // FILTER: koefficienter en gång per steg
                const float keyOct = p.fkey * ((float) note - 60.0f) / 12.0f;
                const float spreadOct = bin * 0.22f;
                const float c1 = p.cut1 * std::exp2 (envOct * fe + keyOct + spreadOct + (d[DstCut1] + d[DstCutBoth]) * 5.0f);
                const float c2 = p.cut2 * std::exp2 (envOct * fe + keyOct - spreadOct + (d[DstCut2] + d[DstCutBoth]) * 5.0f);
                const float r1 = juce::jlimit (0.0f, 1.0f, p.reso1 + d[DstReso] * 0.5f);
                const float r2 = juce::jlimit (0.0f, 1.0f, p.reso2 + d[DstReso] * 0.5f);
                auto setF = [&] (int w, int type, float c, float r)
                {
                    if (type == FLadder) h.lad[w].set (c, fs);
                    else if (type == FComb) h.comb[w].set (c, r, fs);
                    else if (type != FOff) h.svf[w].set (c, r, fs);
                };
                setF (0, p.ftype1, c1, r1);
                setF (1, p.ftype2, c2, r2);
                const float k1 = juce::jmin (3.75f, r1 * 3.6f), k2 = juce::jmin (3.75f, r2 * 3.6f);
                const float noise = juce::jlimit (0.0f, 1.0f, p.noise + d[DstNoise]) * 0.5f;
                h.fbHp.setCutoff (60.0f, fs);

                for (int j = 0; j < n; ++j)
                {
                    float x = bb[j] + randBi (h.rng) * noise;
                    if (heat > 0.001f)
                    {
                        // ÖVERHETTNING: utgången tillbaka in i filtret, genom mättning. Tjockt och smutsigt.
                        x = fastTanh ((x + heat * 0.85f * h.fbHp.hp (h.last)) * heatGain) * heatComp;
                    }
                    if (p.ftype1 != FLadder && p.fdrive > 0.001f) x = fastTanh (x * drive) / drive;
                    float y;
                    if (p.froute == 0)
                        y = h.runFilter (1, p.ftype2, h.runFilter (0, p.ftype1, x, k1, drive), k2, 1.0f);
                    else
                        y = 0.6f * (h.runFilter (0, p.ftype1, x, k1, drive) + h.runFilter (1, p.ftype2, x, k2, 1.0f));
                    h.last = y;
                    bb[j] = y;
                }
            }

            const float pan = juce::jlimit (-1.0f, 1.0f, basePan + panMod);
            const float pa = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            const float panL = std::cos (pa) * juce::MathConstants<float>::sqrt2;
            const float panR = std::sin (pa) * juce::MathConstants<float>::sqrt2;
            const float* right = twoHalves ? buf[1] : buf[0];
            for (int j = 0; j < n; ++j)
            {
                const float g = amp[j] * volMod;
                L[i + j] += buf[0][j] * g * panL;
                R[i + j] += right[j] * g * panR;
            }
        }
    }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kVoices = 8;

    Engine() { WaveBank::get(); }   // tabellerna byggs här, aldrig i ljudtråden

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxBlock);
        for (auto& l : lfoBuf) for (auto& b : l) b.assign ((size_t) maxBlock * 2 + kChunk + 16, 0.0f);
        sendBuf.assign ((size_t) maxBlock + 16, 0.0f);
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            v.fs = (float) sr * 2.0f;
            v.rng = 0x9e3779b9u * (juce::uint32) (i + 1);
            v.half[0].rng = 0x85ebca6bu * (juce::uint32) (i + 1);
            v.half[1].rng = 0xc2b2ae35u * (juce::uint32) (i + 7);
            v.detune = (i % 2 == 0 ? 1.0f : -1.0f) * (0.4f + 0.6f * (float) (i / 2) / 3.0f);
            for (auto& h : v.half) { h.comb[0].init(); h.comb[1].init(); }
            v.aenv.kill(); v.fenv.kill(); v.menv.kill();
        }
        chorus.prepare (sr);
        shimmer.prepare (sr, maxBlock);
        valve.prepare (sr);
        for (auto& d : dc) { d.setCutoff (15.0f, sr); d.z = 0.0f; }
        lfo[0].rng = 4711; lfo[1].rng = 90210;
        numHeld = 0; sustain = false; lastPitch = -1.0f;
    }

    int getLatency() const { return (int) std::round (os != nullptr ? os->getLatencyInSamples() : 0.0f) + valve.getLatency(); }

    //--------------------------------------------------------------------------
    void noteOn (int n, float velocity, const Params& p)
    {
        pushHeld (n);
        if (p.playMode == Poly)
        {
            auto& v = pickVoice (n);
            const float from = (p.glide > 0.001f && lastPitch >= 0.0f) ? lastPitch : -1.0f;
            setPan (v, p);
            v.start (n, velocity, from, true, ++stamp, p.binaural);
        }
        else
        {
            auto& v = voices[0];
            for (int i = 1; i < kVoices; ++i) voices[(size_t) i].release();
            setPan (v, p);
            const bool playing = v.active() && v.held;
            if (p.playMode == Legato && playing)
                v.glideTo (n);                                // bunden: ingen ny attack, bara glid
            else
                v.start (n, velocity, (playing || p.glide > 0.001f) && lastPitch >= 0.0f ? lastPitch : -1.0f, true, ++stamp, p.binaural);
            if (! playing || p.playMode == Mono) v.vel = velocity;
        }
        lastPitch = (float) n;
    }

    void noteOff (int n, const Params& p)
    {
        popHeld (n);
        if (p.playMode == Poly)
        {
            for (auto& v : voices)
                if (v.held && v.note == n)
                {
                    if (sustain) { v.held = false; v.sustained = true; }
                    else v.release();
                }
            return;
        }
        auto& v = voices[0];
        if (v.note != n) return;
        if (numHeld > 0)
        {
            const int back = held[(size_t) numHeld - 1];        // tillbaka till tangenten som fortfarande hålls
            if (p.playMode == Legato) v.glideTo (back);
            else v.start (back, v.vel, (float) n, true, ++stamp, p.binaural);
            lastPitch = (float) back;
        }
        else if (sustain) { v.held = false; v.sustained = true; }
        else v.release();
    }

    void setSustain (bool on)
    {
        sustain = on;
        if (! on)
            for (auto& v : voices)
                if (v.sustained) { v.sustained = false; v.release(); }
    }

    void allOff()
    {
        for (auto& v : voices) v.release();
        numHeld = 0;
    }

    void panic()
    {
        for (auto& v : voices) { v.aenv.kill(); v.fenv.kill(); v.menv.kill(); }
        numHeld = 0;
        chorus.prepare (sr);
        shimmer.reset();
    }

    //--------------------------------------------------------------------------
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Params& p,
                  const std::function<void (const juce::MidiMessage&)>& onController)
    {
        const int n = buffer.getNumSamples();
        buffer.clear();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        juce::dsp::AudioBlock<float> block (buffer);
        auto up = os->processSamplesUp (block.getSubsetChannelBlock (0, (size_t) juce::jmin (2, buffer.getNumChannels())));
        const int un = (int) up.getNumSamples();
        auto* uL = up.getChannelPointer (0);
        auto* uR = up.getNumChannels() > 1 ? up.getChannelPointer (1) : uL;
        juce::FloatVectorOperations::clear (uL, un);
        if (uR != uL) juce::FloatVectorOperations::clear (uR, un);

        const float fsUp = (float) sr * 2.0f;
        for (auto& v : voices)
        {
            v.fenv.set (p.env[0], fsUp);
            v.aenv.set (p.env[1], fsUp);
            v.menv.set (p.env[2], fsUp);
        }

        // LFO 1 och 2, en gång för alla röster. Höger halva får en förskjuten fas (BINAURAL).
        for (int l = 0; l < 2; ++l)
        {
            auto& st = lfo[l];
            const float inc = p.lfoHz[(size_t) l] / fsUp;
            const float offset = p.binaural * 0.25f;
            float* outA = lfoBuf[(size_t) l][0].data();
            float* outB = lfoBuf[(size_t) l][1].data();
            const int wave = p.lfoWave[(size_t) l];
            auto shape = [&st, wave] (float ph) noexcept
            {
                switch (wave)
                {
                    case 0: return std::sin (ph * juce::MathConstants<float>::twoPi);
                    case 1: return 1.0f - 4.0f * std::abs (ph - 0.5f);
                    case 2: return 1.0f - 2.0f * ph;                         // såg ned
                    case 3: return ph < 0.5f ? 1.0f : -1.0f;
                    case 4: return st.next;                                  // slump, hackig
                    default:                                                 // mjuk slump
                    {
                        const float s = 0.5f - 0.5f * std::cos (ph * juce::MathConstants<float>::pi);
                        return st.prev + (st.next - st.prev) * s;
                    }
                }
            };
            for (int i = 0; i < un; ++i)
            {
                st.phase += inc;
                if (st.phase >= 1.0f) { st.phase -= 1.0f; st.prev = st.next; st.next = randBi (st.rng); }
                float phB = st.phase + offset;
                if (phB >= 1.0f) phB -= 1.0f;
                st.smooth  += (shape (st.phase) - st.smooth) * 0.02f;    // rundar hörnen så att inget klickar
                st.smoothB += (shape (phB) - st.smoothB) * 0.02f;
                outA[i] = st.smooth;
                outB[i] = st.smoothB;
            }
        }

        int pos = 0;
        for (const auto meta : midi)
        {
            const int at = juce::jlimit (0, n, meta.samplePosition) * 2;
            renderVoices (uL, uR, pos, at, p);
            pos = at;
            const auto m = meta.getMessage();
            if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
            else if (m.isNoteOff()) noteOff (m.getNoteNumber(), p);
            else if (m.isSustainPedalOn()) setSustain (true);
            else if (m.isSustainPedalOff()) setSustain (false);
            else if (m.isAllNotesOff() || m.isAllSoundOff()) allOff();
            else if (m.isChannelPressure()) aftertouch = (float) m.getChannelPressureValue() / 127.0f;
            else if (onController) onController (m);
        }
        renderVoices (uL, uR, pos, un, p);

        auto downBlock = block.getSubsetChannelBlock (0, (size_t) juce::jmin (2, buffer.getNumChannels()));
        os->processSamplesDown (downBlock);

        // DRIV: mjuk mättnad på hela ljudet
        const float drv = 1.0f + p.drive * 6.0f;
        const float comp = 1.0f / fastTanh (drv * 0.5f) * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            float l = dc[0].hp (outL[i]), r = dc[1].hp (outR[i]);
            if (p.drive > 0.001f) { l = fastTanh (l * drv) * comp; r = fastTanh (r * drv) * comp; }
            outL[i] = l; outR[i] = r;
        }

        // KÖR
        if (p.chorus > 0.001f) chorus.process (outL, outR, n, p.chorus);

        // SHIMMER-RUM
        if (p.rumMix > 0.001f || p.shimmer > 0.001f)
            shimmer.process (outL, outR, n, p.rumMix, p.rumSize, p.shimmer);

        // BREDD: mitt/sida, mer sida = bredare
        if (p.width > 0.001f)
        {
            const float sideGain = 1.0f + 1.2f * p.width, midGain = 1.0f - 0.15f * p.width;
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (outL[i] + outR[i]), s = 0.5f * (outL[i] - outR[i]);
                outL[i] = m * midGain + s * sideGain;
                outR[i] = m * midGain - s * sideGain;
            }
        }

        for (int i = 0; i < n; ++i) { outL[i] *= p.gain; outR[i] *= p.gain; }
        reduction = valve.process (outL, outR, n);
    }

    float reduction = 0.0f;
    std::array<Voice, kVoices> voices {};

private:
    void renderVoices (float* L, float* R, int start, int end, const Params& p)
    {
        if (end <= start) return;
        {
            const float* const lfoPtr[2][2] { { lfoBuf[0][0].data(), lfoBuf[0][1].data() }, { lfoBuf[1][0].data(), lfoBuf[1][1].data() } };
            for (auto& v : voices)
                if (v.active()) v.render (L, R, start, end, p, lfoPtr, aftertouch);
        }
    }

    void setPan (Voice& v, const Params& p)
    {
        v.basePan = p.playMode == Poly ? juce::jlimit (-1.0f, 1.0f, v.detune * p.spread * 0.9f) : 0.0f;
    }

    Voice& pickVoice (int n)
    {
        for (auto& v : voices) if (v.active() && v.note == n) return v;      // samma tangent: återanvänd
        for (auto& v : voices) if (! v.active()) return v;
        Voice* best = nullptr;                                                // annars: äldsta släppta, sist äldsta
        for (auto& v : voices) if (! v.held && (best == nullptr || v.age < best->age)) best = &v;
        if (best == nullptr) for (auto& v : voices) if (best == nullptr || v.age < best->age) best = &v;
        return *best;
    }

    void pushHeld (int n) { popHeld (n); if (numHeld < (int) held.size()) held[(size_t) numHeld++] = n; }
    void popHeld (int n)
    {
        for (int i = 0; i < numHeld; ++i)
            if (held[(size_t) i] == n)
            {
                for (int j = i; j < numHeld - 1; ++j) held[(size_t) j] = held[(size_t) j + 1];
                --numHeld; return;
            }
    }

    struct LfoState { float phase = 0.0f, smooth = 0.0f, smoothB = 0.0f, prev = 0.0f, next = 0.0f; juce::uint32 rng = 4711; };

    double sr = 44100.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    std::array<std::array<std::vector<float>, 2>, 2> lfoBuf;
    std::vector<float> sendBuf;
    std::array<LfoState, 2> lfo {};
    float aftertouch = 0.0f;
    Chorus chorus;
    ShimmerVerb shimmer;
    SafetyValve valve;
    OnePole dc[2];
    std::array<int, 16> held {};
    int numHeld = 0;
    bool sustain = false;
    float lastPitch = -1.0f;
    juce::uint32 stamp = 0;
};
} // namespace masugn
