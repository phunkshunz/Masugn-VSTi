#pragma once

#include "Dsp.h"
#include "Tables.h"
#include <array>
#include <functional>

// MASUGN: ljudmotorn. Åtta röster med tre oscillatorer var. Varje oscillator kan vara
// vågtabell, partikel (granulär), resonator, kärna (FM) eller analog.
// Sedan två filter (serie eller parallellt), tre kuvert, två LFO och en modulationsmatris.
// Rösterna räknas i 2x oversampling. Efter det: driv, kör, eko med tak, rum och SÄKERHETSVENTILEN.
// Allt i ljudtråden är förallokerat.
namespace masugn
{
inline float mtof (float m) noexcept { return 440.0f * std::exp2 ((m - 69.0f) / 12.0f); }

enum OscType { TypeWave, TypeParticle, TypeResonator, TypeKernel, TypeAnalog, NumOscTypes };
enum FilterType { FLadder, FLp12, FBand, FHp, FNotch, FComb, FOff, NumFilterTypes };
enum PlayMode { Poly, Mono, Legato };

// Modulationsmatrisen: källor och mål
enum ModSrc { SrcOff, SrcLfo1, SrcLfo2, SrcEnv3, SrcFEnv, SrcVel, SrcWheel, SrcRandom, SrcKey, SrcAfter, NumSrc };
enum ModDst { DstOff, DstPos1, DstPos2, DstPos3, DstPosAll, DstForm1, DstForm2, DstForm3, DstLevel1, DstLevel2, DstLevel3,
              DstPitch, DstPitch2, DstPitch3, DstCut1, DstCut2, DstCutBoth, DstReso, DstNoise, DstVol, DstPan, NumDst };

constexpr int kModSlots = 4;
constexpr int kChunk = 16;   // kontrollsteg: modulering räknas om var 16:e sampel

//==============================================================================
struct OscP
{
    int type = TypeWave, table = 0, oct = 0;
    float pos = 0.0f, form = 0.0f, semi = 0.0f, fine = 0.0f, level = 0.0f;
};

struct ModSlot { int src = SrcOff, dst = DstOff; float amt = 0.0f; };

struct Params
{
    std::array<OscP, 3> osc {};
    float noise = 0.0f, spread = 0.3f;
    // FILTER
    int ftype1 = FLadder, ftype2 = FOff, froute = 0;
    float cut1 = 2000.0f, reso1 = 0.2f, cut2 = 2000.0f, reso2 = 0.2f, fenv = 0.4f, fkey = 0.3f, fdrive = 0.2f;
    // KUVERT: filter, volym, kuvert 3 (a, d, s, r)
    std::array<std::array<float, 4>, 3> env {{ { 0.002f, 0.3f, 0.0f, 0.2f }, { 0.002f, 0.4f, 0.8f, 0.15f }, { 0.002f, 0.5f, 0.0f, 0.3f } }};
    // LFO
    std::array<int, 2> lfoWave {};
    std::array<float, 2> lfoHz { 2.0f, 0.5f };
    std::array<ModSlot, kModSlots> mod {};
    // SPEL
    int playMode = Poly; float glide = 0.0f;
    // EFFEKT
    float drive = 0.0f, chorus = 0.0f, ekoSec = 0.375f, ekoFb = 0.3f, ekoTone = 0.5f, ekoMix = 0.0f, rumSize = 0.5f, rumMix = 0.0f, gain = 1.0f;
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

    // PARTIKEL
    struct Grain { bool on = false; float ph = 0.0f, inc = 0.0f, age = 0.0f, len = 1.0f; const float* data = nullptr; };
    std::array<Grain, 8> grains {};
    float spawn = 0.0f;

    // RESONATOR: åtta svängande moder
    static constexpr int kModes = 8;
    std::array<float, kModes> y1 {}, y2 {}, c1 {}, c2 {}, b {}, cont {};
    int nModes = 0, burst = 0, burstLen = 1;
    float exLp = 0.0f, contAmt = 0.0f;

    void reset (float startPhase) noexcept
    {
        phase = startPhase; mphase = 0.0f; mfb = 0.0f;
        va.reset (startPhase);
        for (auto& g : grains) g.on = false;
        spawn = 0.0f;
        y1.fill (0.0f); y2.fill (0.0f);
        exLp = 0.0f;
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
                       float* out, int n, float fs, juce::uint32& rng) noexcept
{
    if (level < 0.0005f) return;
    const auto& bank = WaveBank::get();
    const float dt = juce::jlimit (0.0f, 0.45f, hz / fs);
    pos = juce::jlimit (0.0f, 1.0f, pos);
    form = juce::jlimit (0.0f, 1.0f, form);
    table = juce::jlimit (0, kNumTables - 1, table);

    switch (type)
    {
        case TypeWave:
        {
            const int mip = WaveBank::mipFor (hz, fs);
            const float fpos = pos * (float) (kFrames - 1);
            const int f0 = juce::jmin (kFrames - 2, (int) fpos);
            const float fr = fpos - (float) f0;
            const float* a = bank.frame (table, mip, f0);
            const float* b = bank.frame (table, mip, f0 + 1);
            const float w = 0.5f - 0.45f * form;   // BÖJ: trycker ihop första halvan av vågen
            for (int i = 0; i < n; ++i)
            {
                float t = s.phase;
                if (form > 0.001f) t = t < w ? 0.5f * t / w : 0.5f + 0.5f * (t - w) / (1.0f - w);
                out[i] += level * readFrames (a, b, fr, t);
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
                s.b[(size_t) k] = std::sin (w) / std::pow ((float) (k + 1), 0.6f);
                s.cont[(size_t) k] = std::sqrt (2.0f * (1.0f - r));
                s.nModes = k + 1;
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
                out[i] += level * 0.8f * s.va.next (dt, pos, form);
            break;
    }
}

//==============================================================================
struct Voice
{
    std::array<OscVoice, 3> osc {};
    Ladder lad[2];
    Svf svf[2];
    Comb comb[2];
    Env fenv, aenv, menv;
    int note = -1;
    float vel = 1.0f, pitch = 60.0f, target = 60.0f, detune = 0.0f, basePan = 0.0f, rnd = 0.0f, fs = 96000.0f;
    bool held = false, sustained = false;
    juce::uint32 age = 0, rng = 1;

    bool active() const noexcept { return aenv.st != Env::Idle; }

    void start (int n, float velocity, float fromPitch, bool retrigger, juce::uint32 stamp) noexcept
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
                for (int o = 0; o < 3; ++o) osc[(size_t) o].reset ((float) o * 0.31f);
                for (int f = 0; f < 2; ++f) { lad[f].reset(); svf[f].reset(); comb[f].reset(); }
            }
            for (auto& o : osc)
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

    // Lägger till i L/R (oversamplad takt).
    void render (float* L, float* R, int start, int end, const Params& p, const float* lfo1, const float* lfo2,
                 float aftertouch) noexcept
    {
        const float velAmp = (0.55f + 0.45f * vel) * (p.playMode == Poly ? 0.55f : 1.0f);
        const float envOct = p.fenv * 7.0f * (0.75f + 0.25f * vel);
        const float drive = 1.0f + p.fdrive * 4.0f;
        float buf[kChunk];

        for (int i = start; i < end; i += kChunk)
        {
            if (! active()) break;
            const int n = juce::jmin (kChunk, end - i);

            // MODULATION: källornas värden just nu
            float src[NumSrc] {};
            src[SrcLfo1] = lfo1[i]; src[SrcLfo2] = lfo2[i];
            src[SrcEnv3] = menv.v; src[SrcFEnv] = fenv.v; src[SrcVel] = vel;
            src[SrcWheel] = p.modWheel; src[SrcRandom] = rnd;
            src[SrcKey] = juce::jlimit (-1.0f, 1.0f, ((float) note - 60.0f) / 24.0f);
            src[SrcAfter] = aftertouch;
            float d[NumDst] {};
            for (const auto& m : p.mod)
                if (m.src > SrcOff && m.src < NumSrc && m.dst > DstOff && m.dst < NumDst)
                    d[m.dst] += m.amt * src[m.src];

            // GLID
            const float gc = p.glide > 0.001f ? 1.0f - std::exp (-3.0f * (float) n / (p.glide * fs)) : 1.0f;
            pitch += (target - pitch) * gc;
            const float basePitch = pitch + p.bend + d[DstPitch] * 12.0f;

            std::fill (buf, buf + n, 0.0f);
            for (int o = 0; o < 3; ++o)
            {
                const auto& op = p.osc[(size_t) o];
                const float level = juce::jlimit (0.0f, 1.0f, op.level + d[DstLevel1 + o]);
                if (level < 0.0005f) continue;
                const float extra = o == 1 ? d[DstPitch2] * 12.0f : (o == 2 ? d[DstPitch3] * 12.0f : 0.0f);
                const float pm = basePitch + 12.0f * (float) op.oct + op.semi + op.fine * 0.01f + extra
                               + detune * p.spread * 0.08f * (float) (o - 1);
                const float pos = op.pos + d[DstPos1 + o] + d[DstPosAll];
                const float form = op.form + d[DstForm1 + o];
                renderOsc (osc[(size_t) o], op.type, op.table, mtof (pm), pos, form, level * 0.6f, buf, n, fs, rng);
            }

            // FILTER: koefficienter en gång per steg
            const float fe = fenv.v;
            const float keyOct = p.fkey * ((float) note - 60.0f) / 12.0f;
            const float c1 = p.cut1 * std::exp2 (envOct * fe + keyOct + (d[DstCut1] + d[DstCutBoth]) * 5.0f);
            const float c2 = p.cut2 * std::exp2 (envOct * fe + keyOct + (d[DstCut2] + d[DstCutBoth]) * 5.0f);
            const float r1 = juce::jlimit (0.0f, 1.0f, p.reso1 + d[DstReso] * 0.5f);
            const float r2 = juce::jlimit (0.0f, 1.0f, p.reso2 + d[DstReso] * 0.5f);
            auto setF = [&] (int w, int type, float c, float r)
            {
                if (type == FLadder) lad[w].set (c, fs);
                else if (type == FComb) comb[w].set (c, r, fs);
                else if (type != FOff) svf[w].set (c, r, fs);
            };
            setF (0, p.ftype1, c1, r1);
            setF (1, p.ftype2, c2, r2);
            const float k1 = juce::jmin (3.75f, r1 * 3.6f), k2 = juce::jmin (3.75f, r2 * 3.6f);

            const float noise = juce::jlimit (0.0f, 1.0f, p.noise + d[DstNoise]) * 0.5f;
            const float volMod = juce::jlimit (0.0f, 2.0f, 1.0f + d[DstVol]);
            const float pan = juce::jlimit (-1.0f, 1.0f, basePan + d[DstPan]);
            const float pa = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            const float panL = std::cos (pa) * juce::MathConstants<float>::sqrt2;
            const float panR = std::sin (pa) * juce::MathConstants<float>::sqrt2;

            for (int j = 0; j < n; ++j)
            {
                float x = buf[j] + randBi (rng) * noise;
                if (p.ftype1 != FLadder && p.fdrive > 0.001f) x = fastTanh (x * drive) / drive;
                float y;
                if (p.froute == 0)
                    y = runFilter (1, p.ftype2, runFilter (0, p.ftype1, x, k1, drive), k2, 1.0f);
                else
                    y = 0.6f * (runFilter (0, p.ftype1, x, k1, drive) + runFilter (1, p.ftype2, x, k2, 1.0f));

                fenv.next();
                menv.next();
                y *= aenv.next() * velAmp * volMod;
                L[i + j] += y * panL;
                R[i + j] += y * panR;
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
        for (auto& b : lfoBuf) b.assign ((size_t) maxBlock * 2 + kChunk + 16, 0.0f);
        sendBuf.assign ((size_t) maxBlock + 16, 0.0f);
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            v.fs = (float) sr * 2.0f;
            v.rng = 0x9e3779b9u * (juce::uint32) (i + 1);
            v.detune = (i % 2 == 0 ? 1.0f : -1.0f) * (0.4f + 0.6f * (float) (i / 2) / 3.0f);
            v.comb[0].init(); v.comb[1].init();
            v.aenv.kill(); v.fenv.kill(); v.menv.kill();
        }
        chorus.prepare (sr);
        delay.prepare (sr);
        reverb.setSampleRate (sr);
        reverb.reset();
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
            v.start (n, velocity, from, true, ++stamp);
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
                v.start (n, velocity, (playing || p.glide > 0.001f) && lastPitch >= 0.0f ? lastPitch : -1.0f, true, ++stamp);
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
            else v.start (back, v.vel, (float) n, true, ++stamp);
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
        delay.prepare (sr);
        chorus.prepare (sr);
        reverb.reset();
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

        // LFO 1 och 2, en gång för alla röster
        for (int l = 0; l < 2; ++l)
        {
            auto& st = lfo[l];
            const float inc = p.lfoHz[(size_t) l] / fsUp;
            float* out = lfoBuf[(size_t) l].data();
            for (int i = 0; i < un; ++i)
            {
                st.phase += inc;
                if (st.phase >= 1.0f) { st.phase -= 1.0f; st.prev = st.next; st.next = randBi (st.rng); }
                float v = 0.0f;
                switch (p.lfoWave[(size_t) l])
                {
                    case 0: v = std::sin (st.phase * juce::MathConstants<float>::twoPi); break;
                    case 1: v = 1.0f - 4.0f * std::abs (st.phase - 0.5f); break;
                    case 2: v = 1.0f - 2.0f * st.phase; break;                        // såg ned
                    case 3: v = st.phase < 0.5f ? 1.0f : -1.0f; break;
                    case 4: v = st.next; break;                                         // slump, hackig
                    default:                                                            // mjuk slump
                    {
                        const float s = 0.5f - 0.5f * std::cos (st.phase * juce::MathConstants<float>::pi);
                        v = st.prev + (st.next - st.prev) * s;
                        break;
                    }
                }
                st.smooth += (v - st.smooth) * 0.02f;   // rundar hörnen så att inget klickar
                out[i] = st.smooth;
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

        // EKO: dub-eko med tak på återkopplingen, så att det aldrig skenar
        if (p.ekoMix > 0.001f)
        {
            float* send = sendBuf.data();
            for (int i = 0; i < n; ++i) send[i] = 0.5f * (outL[i] + outR[i]);
            delay.process (send, outL, outR, n, p.ekoSec, juce::jmin (0.7f, p.ekoFb), p.ekoTone, 0.12f, p.ekoMix);
        }

        // RUM
        if (p.rumMix > 0.001f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.3f + 0.65f * p.rumSize;
            rp.damping = 0.5f;
            rp.wetLevel = p.rumMix * 0.6f;
            rp.dryLevel = 1.0f;
            rp.width = 1.0f;
            reverb.setParameters (rp);
            reverb.processStereo (outL, outR, n);
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
        for (auto& v : voices)
            if (v.active()) v.render (L, R, start, end, p, lfoBuf[0].data(), lfoBuf[1].data(), aftertouch);
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

    struct LfoState { float phase = 0.0f, smooth = 0.0f, prev = 0.0f, next = 0.0f; juce::uint32 rng = 4711; };

    double sr = 44100.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    std::array<std::vector<float>, 2> lfoBuf;
    std::vector<float> sendBuf;
    std::array<LfoState, 2> lfo {};
    float aftertouch = 0.0f;
    Chorus chorus;
    DubDelay delay;
    juce::Reverb reverb;
    SafetyValve valve;
    OnePole dc[2];
    std::array<int, 16> held {};
    int numHeld = 0;
    bool sustain = false;
    float lastPitch = -1.0f;
    juce::uint32 stamp = 0;
};
} // namespace masugn
