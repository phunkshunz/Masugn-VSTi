#pragma once

#include "Engine.h"

// MASUGN: alla rattar på ett ställe. Pluginet bygger sina parametrar härifrån,
// presets och testprogrammet använder samma tabell.
namespace masugn
{
enum P
{
    // OSC 1, 2, 3 (samma ordning för alla tre, OscStride steg mellan dem)
    O1Type, O1Table, O1Mut, O1Pos, O1Form, O1MAmt, O1Oct, O1Semi, O1Fine, O1Level,
    O2Type, O2Table, O2Mut, O2Pos, O2Form, O2MAmt, O2Oct, O2Semi, O2Fine, O2Level,
    O3Type, O3Table, O3Mut, O3Pos, O3Form, O3MAmt, O3Oct, O3Semi, O3Fine, O3Level,
    Noise, Spread, Binaural,
    // FILTER
    F1Type, Cut1, Reso1, F2Type, Cut2, Reso2, FRoute, FEnv, FKey, FDrive, Heat,
    // KUVERT: filter, volym, kuvert 3
    FA, FD, FS, FR, AA, AD, AS, AR, MA, MD, MS, MR,
    // LFO
    L1Wave, L1Sync, L1Rate, L1Div, L2Wave, L2Sync, L2Rate, L2Div,
    // MOD
    M1Src, M1Dst, M1Amt, M2Src, M2Dst, M2Amt, M3Src, M3Dst, M3Amt, M4Src, M4Dst, M4Amt,
    // SPEL
    Mode, Glide,
    // EFFEKT
    Drive, Chorus, Shimmer, RumSize, RumMix, Width,
    // SMÄLT (mellan ljud A och B) och volym
    Smalt, Vol,
    NumParams
};

constexpr int OscStride = O2Type - O1Type;
// En oscillators ratt: oscP (1, O1Pos) = O2Pos
constexpr P oscP (int o, P first) { return (P) ((int) first + o * OscStride); }
constexpr P modP (int slot, P first) { return (P) ((int) first + slot * 3); }
constexpr P lfoP (int l, P first) { return (P) ((int) first + l * 4); }

enum class Kind { Float, Int, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;     // UTF-8
    Kind kind;
    float min, max, def, centre; // centre: värdet i mitten av ratten (för log-rattar), 0 = linjär
    const char* unit;
    std::initializer_list<const char*> choices;
};

#define MASUGN_TYPES   { "Vågtabell", "Partikel", "Resonator", "Kärna", "Analog" }
#define MASUGN_TABLES  { "Grund", "Puls", "Synk", "Veck", "Vokal", "Trappa", "Glas", "Spektrum", "Metall", "Orgel", "Kvint", "CZ Reso" }
#define MASUGN_MUTS    { "Ingen", "FM", "Synk", "Veck", "Super", "Fasdiff" }
#define MASUGN_FILTERS { "Ladder 24", "LP 12", "Band", "HP", "Notch", "Kam", "Av" }
#define MASUGN_LFOS    { "Sinus", "Triangel", "Såg", "Fyrkant", "Slump", "Mjuk slump" }
#define MASUGN_DIVS    { "4 takter", "2 takter", "1 takt", "1/2", "1/4", "1/8", "1/16", "1/4 T", "1/8 T", "1/8 punkt" }
#define MASUGN_SRC     { "Av", "LFO 1", "LFO 2", "Kuvert 3", "Filterkuvert", "Anslag", "Modhjul", "Slump", "Tangent", "Aftertouch" }
#define MASUGN_DST     { "Av", "OSC 1 pos", "OSC 2 pos", "OSC 3 pos", "Alla pos", "OSC 1 form", "OSC 2 form", "OSC 3 form", \
                         "OSC 1 nivå", "OSC 2 nivå", "OSC 3 nivå", "Ton", "OSC 2 ton", "OSC 3 ton", "Cutoff 1", "Cutoff 2", \
                         "Båda cutoff", "Resonans", "Brus", "Volym", "Panorering", "OSC 1 mutation", "OSC 2 mutation", "OSC 3 mutation" }

inline float lfoDivBeats (int i)
{
    static const float b[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 2.0f / 3.0f, 1.0f / 3.0f, 0.75f };
    return b[juce::jlimit (0, 9, i)];
}

inline const std::array<ParamDef, NumParams>& paramDefs()
{
    static const std::array<ParamDef, NumParams> d {{
        { "o1type",  "OSC 1 TYP",  Kind::Choice, 0, 4, 0, 0, "", MASUGN_TYPES },
        { "o1table", "OSC 1 TABELL", Kind::Choice, 0, 11, 0, 0, "", MASUGN_TABLES },
        { "o1mut",   "OSC 1 MUTATOR", Kind::Choice, 0, 5, 0, 0, "", MASUGN_MUTS },
        { "o1pos",   "OSC 1 POS",  Kind::Float, 0, 1, 0.3f, 0, "", {} },
        { "o1form",  "OSC 1 FORM", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o1mamt",  "OSC 1 MUTATION", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o1oct",   "OSC 1 OKTAV",Kind::Int,  -3, 2, 0, 0, "", {} },
        { "o1semi",  "OSC 1 TON",  Kind::Int, -24, 24, 0, 0, "st", {} },
        { "o1fine",  "OSC 1 FIN",  Kind::Float, -50, 50, 0, 0, "c", {} },
        { "o1level", "OSC 1 NIVÅ", Kind::Float, 0, 1, 0.8f, 0, "", {} },

        { "o2type",  "OSC 2 TYP",  Kind::Choice, 0, 4, 0, 0, "", MASUGN_TYPES },
        { "o2table", "OSC 2 TABELL", Kind::Choice, 0, 11, 0, 0, "", MASUGN_TABLES },
        { "o2mut",   "OSC 2 MUTATOR", Kind::Choice, 0, 5, 0, 0, "", MASUGN_MUTS },
        { "o2pos",   "OSC 2 POS",  Kind::Float, 0, 1, 0.5f, 0, "", {} },
        { "o2form",  "OSC 2 FORM", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o2mamt",  "OSC 2 MUTATION", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o2oct",   "OSC 2 OKTAV",Kind::Int,  -3, 2, 0, 0, "", {} },
        { "o2semi",  "OSC 2 TON",  Kind::Int, -24, 24, 0, 0, "st", {} },
        { "o2fine",  "OSC 2 FIN",  Kind::Float, -50, 50, 7, 0, "c", {} },
        { "o2level", "OSC 2 NIVÅ", Kind::Float, 0, 1, 0.0f, 0, "", {} },

        { "o3type",  "OSC 3 TYP",  Kind::Choice, 0, 4, 4, 0, "", MASUGN_TYPES },
        { "o3table", "OSC 3 TABELL", Kind::Choice, 0, 11, 0, 0, "", MASUGN_TABLES },
        { "o3mut",   "OSC 3 MUTATOR", Kind::Choice, 0, 5, 0, 0, "", MASUGN_MUTS },
        { "o3pos",   "OSC 3 POS",  Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o3form",  "OSC 3 FORM", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o3mamt",  "OSC 3 MUTATION", Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "o3oct",   "OSC 3 OKTAV",Kind::Int,  -3, 2, -1, 0, "", {} },
        { "o3semi",  "OSC 3 TON",  Kind::Int, -24, 24, 0, 0, "st", {} },
        { "o3fine",  "OSC 3 FIN",  Kind::Float, -50, 50, 0, 0, "c", {} },
        { "o3level", "OSC 3 NIVÅ", Kind::Float, 0, 1, 0.0f, 0, "", {} },

        { "noise",   "BRUS",       Kind::Float, 0, 1, 0.0f, 0, "", {} },
        { "spread",  "SPRIDNING",  Kind::Float, 0, 1, 0.3f, 0, "", {} },
        { "binaural","BINAURAL",   Kind::Float, 0, 1, 0.5f, 0, "", {} },

        { "f1type",  "FILTER 1",   Kind::Choice, 0, 6, 0, 0, "", MASUGN_FILTERS },
        { "cut1",    "CUTOFF 1",   Kind::Float, 30, 18000, 1500, 800, "Hz", {} },
        { "reso1",   "RESO 1",     Kind::Float, 0, 1, 0.2f, 0, "", {} },
        { "f2type",  "FILTER 2",   Kind::Choice, 0, 6, 6, 0, "", MASUGN_FILTERS },
        { "cut2",    "CUTOFF 2",   Kind::Float, 30, 18000, 3000, 800, "Hz", {} },
        { "reso2",   "RESO 2",     Kind::Float, 0, 1, 0.2f, 0, "", {} },
        { "froute",  "KOPPLING",   Kind::Choice, 0, 1, 0, 0, "", { "Serie", "Parallell" } },
        { "fenv",    "ENV-MOD",    Kind::Float, -1, 1, 0.3f, 0, "", {} },
        { "fkey",    "TANGENT",    Kind::Float, 0, 1, 0.3f, 0, "", {} },
        { "fdrive",  "DRIV IN",    Kind::Float, 0, 1, 0.1f, 0, "", {} },
        { "heat",    "HETTA",      Kind::Float, 0, 1, 0.0f, 0, "", {} },

        { "fa",      "F ATTACK",   Kind::Float, 0.001f, 6.0f, 0.002f, 0.2f, "s", {} },
        { "fd",      "F DECAY",    Kind::Float, 0.005f, 8.0f, 0.4f, 0.5f, "s", {} },
        { "fs",      "F SUSTAIN",  Kind::Float, 0, 1, 0.2f, 0, "", {} },
        { "fr",      "F RELEASE",  Kind::Float, 0.005f, 10.0f, 0.3f, 0.6f, "s", {} },
        { "aa",      "ATTACK",     Kind::Float, 0.001f, 6.0f, 0.003f, 0.2f, "s", {} },
        { "ad",      "DECAY",      Kind::Float, 0.005f, 8.0f, 0.5f, 0.5f, "s", {} },
        { "as",      "SUSTAIN",    Kind::Float, 0, 1, 0.8f, 0, "", {} },
        { "ar",      "RELEASE",    Kind::Float, 0.005f, 10.0f, 0.3f, 0.6f, "s", {} },
        { "ma",      "3 ATTACK",   Kind::Float, 0.001f, 8.0f, 0.5f, 0.3f, "s", {} },
        { "md",      "3 DECAY",    Kind::Float, 0.005f, 10.0f, 2.0f, 0.8f, "s", {} },
        { "ms",      "3 SUSTAIN",  Kind::Float, 0, 1, 0.5f, 0, "", {} },
        { "mr",      "3 RELEASE",  Kind::Float, 0.005f, 10.0f, 1.0f, 0.8f, "s", {} },

        { "l1wave",  "LFO 1 FORM", Kind::Choice, 0, 5, 0, 0, "", MASUGN_LFOS },
        { "l1sync",  "LFO 1 TAKT", Kind::Bool, 0, 1, 1, 0, "", {} },
        { "l1rate",  "LFO 1 FART", Kind::Float, 0.02f, 20.0f, 1.0f, 1.0f, "Hz", {} },
        { "l1div",   "LFO 1 TAKTDEL", Kind::Choice, 0, 9, 3, 0, "", MASUGN_DIVS },
        { "l2wave",  "LFO 2 FORM", Kind::Choice, 0, 5, 5, 0, "", MASUGN_LFOS },
        { "l2sync",  "LFO 2 TAKT", Kind::Bool, 0, 1, 0, 0, "", {} },
        { "l2rate",  "LFO 2 FART", Kind::Float, 0.02f, 20.0f, 0.3f, 1.0f, "Hz", {} },
        { "l2div",   "LFO 2 TAKTDEL", Kind::Choice, 0, 9, 1, 0, "", MASUGN_DIVS },

        { "m1src",   "MOD 1 KÄLLA", Kind::Choice, 0, 9, 0, 0, "", MASUGN_SRC },
        { "m1dst",   "MOD 1 MÅL",   Kind::Choice, 0, 23, 0, 0, "", MASUGN_DST },
        { "m1amt",   "MOD 1 MÄNGD", Kind::Float, -1, 1, 0, 0, "", {} },
        { "m2src",   "MOD 2 KÄLLA", Kind::Choice, 0, 9, 0, 0, "", MASUGN_SRC },
        { "m2dst",   "MOD 2 MÅL",   Kind::Choice, 0, 23, 0, 0, "", MASUGN_DST },
        { "m2amt",   "MOD 2 MÄNGD", Kind::Float, -1, 1, 0, 0, "", {} },
        { "m3src",   "MOD 3 KÄLLA", Kind::Choice, 0, 9, 0, 0, "", MASUGN_SRC },
        { "m3dst",   "MOD 3 MÅL",   Kind::Choice, 0, 23, 0, 0, "", MASUGN_DST },
        { "m3amt",   "MOD 3 MÄNGD", Kind::Float, -1, 1, 0, 0, "", {} },
        { "m4src",   "MOD 4 KÄLLA", Kind::Choice, 0, 9, 0, 0, "", MASUGN_SRC },
        { "m4dst",   "MOD 4 MÅL",   Kind::Choice, 0, 23, 0, 0, "", MASUGN_DST },
        { "m4amt",   "MOD 4 MÄNGD", Kind::Float, -1, 1, 0, 0, "", {} },

        { "mode",    "SPELSÄTT",   Kind::Choice, 0, 2, 0, 0, "", { "Poly", "Mono", "Bunden" } },
        { "glide",   "GLID",       Kind::Float, 0, 2, 0, 0.2f, "s", {} },

        { "drive",   "DRIV",       Kind::Float, 0, 1, 0, 0, "", {} },
        { "chorus",  "KÖR",        Kind::Float, 0, 1, 0, 0, "", {} },
        { "shimmer", "SHIMMER",    Kind::Float, 0, 1, 0, 0, "", {} },
        { "rumSize", "STORLEK",    Kind::Float, 0, 1, 0.6f, 0, "", {} },
        { "rumMix",  "RUM",        Kind::Float, 0, 1, 0.2f, 0, "", {} },
        { "width",   "BREDD",      Kind::Float, 0, 1, 0.2f, 0, "", {} },
        { "smalt",   "SMÄLT",      Kind::Float, 0, 1, 0, 0, "", {} },
        { "vol",     "VOLYM",      Kind::Float, -24, 12, 0, 0, "dB", {} },
    }};
    return d;
}

// Kontroll: varje ratt i listan ovan måste ha ett id (fångar en bortglömd rad)
inline bool paramTableComplete()
{
    for (const auto& d : paramDefs()) if (d.id == nullptr || d.name == nullptr) return false;
    return true;
}

inline int indexOf (const juce::String& id)
{
    const auto& d = paramDefs();
    for (int i = 0; i < NumParams; ++i) if (id == d[(size_t) i].id) return i;
    return -1;
}

// Från rattvärden (riktiga enheter) till motorns Params.
inline Params toParams (const std::array<float, NumParams>& v, double bpm)
{
    Params p;
    auto i = [&] (int k) { return (int) std::round (v[(size_t) k]); };
    for (int o = 0; o < 3; ++o)
    {
        auto& op = p.osc[(size_t) o];
        op.type = i (oscP (o, O1Type)); op.table = i (oscP (o, O1Table));
        op.pos = v[oscP (o, O1Pos)]; op.form = v[oscP (o, O1Form)];
        op.oct = i (oscP (o, O1Oct)); op.semi = (float) i (oscP (o, O1Semi)); op.fine = v[oscP (o, O1Fine)];
        op.level = v[oscP (o, O1Level)];
        op.mut = i (oscP (o, O1Mut)); op.mamt = v[oscP (o, O1MAmt)];
    }
    p.noise = v[Noise]; p.spread = v[Spread]; p.binaural = v[Binaural];
    p.ftype1 = i (F1Type); p.cut1 = v[Cut1]; p.reso1 = v[Reso1];
    p.ftype2 = i (F2Type); p.cut2 = v[Cut2]; p.reso2 = v[Reso2];
    p.froute = i (FRoute); p.fenv = v[FEnv]; p.fkey = v[FKey]; p.fdrive = v[FDrive]; p.heat = v[Heat];
    for (int e = 0; e < 3; ++e)
        for (int s = 0; s < 4; ++s) p.env[(size_t) e][(size_t) s] = v[(size_t) (FA + e * 4 + s)];

    const double beatSec = 60.0 / juce::jlimit (30.0, 300.0, bpm);
    for (int l = 0; l < 2; ++l)
    {
        p.lfoWave[(size_t) l] = i (lfoP (l, L1Wave));
        p.lfoHz[(size_t) l] = v[lfoP (l, L1Sync)] > 0.5f ? (float) (1.0 / (beatSec * lfoDivBeats (i (lfoP (l, L1Div))))) : v[lfoP (l, L1Rate)];
    }
    for (int m = 0; m < kModSlots; ++m)
    {
        p.mod[(size_t) m].src = i (modP (m, M1Src));
        p.mod[(size_t) m].dst = i (modP (m, M1Dst));
        p.mod[(size_t) m].amt = v[modP (m, M1Amt)];
    }
    p.playMode = i (Mode); p.glide = v[Glide];
    p.drive = v[Drive]; p.chorus = v[Chorus];
    p.shimmer = v[Shimmer]; p.rumSize = v[RumSize]; p.rumMix = v[RumMix]; p.width = v[Width];
    p.gain = juce::Decibels::decibelsToGain (v[Vol]);
    return p;
}

// Rattar som vrids logaritmiskt (tider och frekvenser): blandas på log-skala
inline bool isLogParam (int k)
{
    return k == Cut1 || k == Cut2 || k == FA || k == FD || k == FR || k == AA || k == AD || k == AR
        || k == MA || k == MD || k == MR || k == L1Rate || k == L2Rate || k == Glide;
}

// SMÄLT: blandar ljud A och B. 0 = bara A, 1 = bara B. Väljare byter vid mitten.
inline std::array<float, NumParams> meltValues (const std::array<float, NumParams>& a, const std::array<float, NumParams>& b, float m)
{
    if (m < 0.0005f) return a;
    auto out = a;
    for (int k = 0; k < NumParams; ++k)
    {
        if (k == Smalt || k == Vol) continue;
        const auto& d = paramDefs()[(size_t) k];
        const float x = a[(size_t) k], y = b[(size_t) k];
        if (d.kind == Kind::Float)
        {
            if (isLogParam (k) && x > 0.0f && y > 0.0f) out[(size_t) k] = std::exp (std::log (x) + (std::log (y) - std::log (x)) * m);
            else out[(size_t) k] = x + (y - x) * m;
        }
        else out[(size_t) k] = m < 0.5f ? x : y;
    }
    // modulationsmängd: om kopplingen skiljer sig tonas den in eller ut i stället för att hoppa
    for (int s = 0; s < kModSlots; ++s)
    {
        const int src = modP (s, M1Src), dst = modP (s, M1Dst), amt = modP (s, M1Amt);
        const bool same = std::round (a[(size_t) src]) == std::round (b[(size_t) src]) && std::round (a[(size_t) dst]) == std::round (b[(size_t) dst]);
        if (! same) out[(size_t) amt] = m < 0.5f ? a[(size_t) amt] * (1.0f - 2.0f * m) : b[(size_t) amt] * (2.0f * m - 1.0f);
    }
    return out;
}

inline std::array<float, NumParams> defaultValues()
{
    std::array<float, NumParams> v {};
    for (int k = 0; k < NumParams; ++k) v[(size_t) k] = paramDefs()[(size_t) k].def;
    return v;
}
} // namespace masugn
