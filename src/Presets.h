#pragma once

#include "ParamTable.h"
#include <vector>

// MASUGN: ljudbiblioteket, GJUTFORMARNA. Sex grupper. Varje ljud är ändringar från grundläget,
// och har ett B-ljud som SMÄLT-ratten glider mot.
// Volymen (VOL) är uppmätt med tools/MasugnTest så att alla låter ungefär lika starkt.
namespace masugn
{
struct Preset
{
    juce::String group, name;
    std::vector<std::pair<P, float>> values;    // ljud A: ändringar från grundläget
    std::vector<std::pair<P, float>> valuesB;   // ljud B: ändringar från A (för SMÄLT)
};

inline const juce::StringArray& presetGroups()
{
    static const juce::StringArray g { "RAVE", "YTA", "MOLN", "KLANG", juce::String::fromUTF8 ("DRÖN"), "LEAD" };
    return g;
}

inline juce::String groupInfo (const juce::String& group)
{
    if (group == "RAVE")  return juce::String::fromUTF8 ("Supervågor, stabs och hoovers");
    if (group == "YTA")   return juce::String::fromUTF8 ("Breda mattor som andas");
    if (group == "MOLN")  return juce::String::fromUTF8 ("Granulära moln och partiklar");
    if (group == "KLANG") return juce::String::fromUTF8 ("Klockor, plock och glas");
    if (group == juce::String::fromUTF8 ("DRÖN")) return juce::String::fromUTF8 ("Långa, glödande drönare");
    if (group == "LEAD")  return juce::String::fromUTF8 ("Leads som skär och sjunger");
    return {};
}

// Uppmätta volymer (dB), samma ordning som listan nedan. Fylls i av tools/MasugnTest.
inline const std::vector<float>& presetTrims()
{
    static const std::vector<float> t {
        // MASUGN_TRIMS_BEGIN
        3.7f, // Supersmälta
        1.8f, // Hoover 2026
        8.3f, // Ravestab Ugn
        -1.1f, // Synkpump
        2.1f, // Fasdiff Stab
        10.1f, // Ravepiano Glöd
        -2.5f, // Glödsåg
        7.8f, // Kvintregn
        9.9f, // Smältvärme
        12.6f, // Vokalkör
        10.7f, // Glödande Glas
        1.8f, // Orgeldimma
        9.1f, // Spektral Andning
        7.1f, // Nattskiftet
        -2.6f, // Resonansmatta
        7.0f, // Varvsgryning
        8.6f, // Partikelmoln
        15.9f, // Ånghall
        0.4f, // Gnistregn
        10.3f, // Glasdamm
        11.7f, // Vokalsvärm
        6.8f, // Kornig Orgel
        2.4f, // Slagghög
        -2.3f, // Trappmoln
        -3.8f, // Stålklocka
        -0.9f, // Marimba Hall
        -3.5f, // Knäppt Sträng
        -0.0f, // FM Piano
        -1.9f, // DX Klocka
        3.3f, // Glasplock
        -2.1f, // Kalimba
        -2.1f, // Smidesstäd
        -0.7f, // Masugnen
        -2.6f, // Kärnklyvning
        16.7f, // Svart Glas
        -5.0f, // Tung Klocka
        16.0f, // Gruvgång
        8.4f, // Ångvissla
        -5.2f, // Smältdegel
        -5.6f, // Ekolod
        2.0f, // Glödtråd
        1.7f, // Synksvep
        1.4f, // Glaspipa
        0.8f, // CZ Signal
        -4.9f, // Kärnlead
        -2.1f, // Fasdiff Lead
        5.2f, // Vokallead
        -0.4f, // Hetlead
        // MASUGN_TRIMS_END
    };
    return t;
}

namespace build
{
using V = std::vector<std::pair<P, float>>;
inline V operator+ (V a, const V& b) { a.insert (a.end(), b.begin(), b.end()); return a; }

// en oscillator: o = 0..2
inline V osc (int o, int type, int table, float pos, float form, float level, int oct = 0, int semi = 0, float fine = 0.0f)
{
    return { { oscP (o, O1Type), (float) type }, { oscP (o, O1Table), (float) table }, { oscP (o, O1Pos), pos },
             { oscP (o, O1Form), form }, { oscP (o, O1Level), level }, { oscP (o, O1Oct), (float) oct },
             { oscP (o, O1Semi), (float) semi }, { oscP (o, O1Fine), fine } };
}
inline V W  (int o, int table, float pos, float form, float lvl, int oct = 0, int semi = 0, float fine = 0) { return osc (o, TypeWave, table, pos, form, lvl, oct, semi, fine); }
inline V PT (int o, int table, float pos, float form, float lvl, int oct = 0, int semi = 0, float fine = 0) { return osc (o, TypeParticle, table, pos, form, lvl, oct, semi, fine); }
inline V RS (int o, float material, float klang, float lvl, int oct = 0, int semi = 0, float fine = 0) { return osc (o, TypeResonator, 0, material, klang, lvl, oct, semi, fine); }
inline V K  (int o, float index, float ratio, float lvl, int oct = 0, int semi = 0, float fine = 0) { return osc (o, TypeKernel, 0, index, ratio, lvl, oct, semi, fine); }
inline V A  (int o, float wave, float skew, float lvl, int oct = 0, int semi = 0, float fine = 0) { return osc (o, TypeAnalog, 0, wave, skew, lvl, oct, semi, fine); }
// mutator på en oscillator
inline V MU (int o, int mut, float amt) { return { { oscP (o, O1Mut), (float) mut }, { oscP (o, O1MAmt), amt } }; }
// en enskild oscillatorratt (för B)
inline V OP (int o, P first, float v) { return { { oscP (o, first), v } }; }

inline V F (int t1, float c1, float r1, int t2 = FOff, float c2 = 3000.0f, float r2 = 0.2f, int route = 0)
{
    return { { F1Type, (float) t1 }, { Cut1, c1 }, { Reso1, r1 }, { F2Type, (float) t2 }, { Cut2, c2 }, { Reso2, r2 }, { FRoute, (float) route } };
}
// kuvert: 0 = filter, 1 = volym, 2 = kuvert 3
inline V E (int e, float a, float d, float s, float r)
{
    const P b = (P) (FA + e * 4);
    return { { b, a }, { (P) (b + 1), d }, { (P) (b + 2), s }, { (P) (b + 3), r } };
}
// LFO: synkad -> värdet är taktdel (index), annars Hz
inline V L (int l, int wave, bool sync, float val)
{
    return { { lfoP (l, L1Wave), (float) wave }, { lfoP (l, L1Sync), sync ? 1.0f : 0.0f }, { sync ? lfoP (l, L1Div) : lfoP (l, L1Rate), val } };
}
inline V M (int slot, int src, int dst, float amt)
{
    return { { modP (slot, M1Src), (float) src }, { modP (slot, M1Dst), (float) dst }, { modP (slot, M1Amt), amt } };
}
inline V MAmt (int slot, float amt) { return { { modP (slot, M1Amt), amt } }; }

// taktdelar
constexpr float d4bars = 0, d2bars = 1, d1bar = 2, d2 = 3, d4 = 4, d8 = 5, d16 = 6;
// tabeller
enum { Grund, Puls, Synk, Veck, Vokal, Trappa, Glas, Spektrum, Metall, Orgel, Kvint, CZ };
// LFO-former
enum { Sin, Tri, Saw, Sq, Rnd, Smooth };
// FM-kvoter (form-värden för kärnan)
constexpr float r05 = 0.0f, r1 = 0.09f, r141 = 0.18f, r2 = 0.27f, r3 = 0.36f, r35 = 0.45f, r4 = 0.55f, r5 = 0.64f, r7 = 0.82f;
// Grund-tabellens platser: sinus 0, triangel 0.33, såg 0.67, fyrkant 1
constexpr float sine = 0.0f, tri = 0.33f, saw = 0.67f, square = 1.0f;
} // namespace build

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list = []
    {
        using namespace build;
        std::vector<Preset> l;
        auto add = [&l] (const char* g, const char* n, V a, V b)
        {
            l.push_back ({ juce::String::fromUTF8 (g), juce::String::fromUTF8 (n), std::move (a), std::move (b) });
        };
        const V mono  { { Mode, 1 }, { Spread, 0 } };
        const V bound { { Mode, 2 }, { Spread, 0 } };

        //======================================================================  RAVE
        add ("RAVE", "Supersmälta",
             W (0, Grund, saw, 0, 0.75f) + MU (0, MutSuper, 0.6f) + W (1, Grund, saw, 0, 0.4f, -1) + MU (1, MutSuper, 0.4f)
             + F (FLadder, 2500, 0.2f) + E (0, 0.002f, 0.5f, 0.3f, 0.3f) + E (1, 0.003f, 0.4f, 0.85f, 0.35f)
             + V { { FEnv, 0.35f }, { Binaural, 0.7f }, { Chorus, 0.3f }, { RumMix, 0.25f }, { Width, 0.3f } },
             V { { Cut1, 6000 }, { Heat, 0.3f }, { Shimmer, 0.3f } } + MU (0, MutSuper, 0.95f) + MU (1, MutSuper, 0.8f));
        add ("RAVE", "Hoover 2026",
             mono + V { { Glide, 0.12f } } + W (0, Puls, 0.45f, 0, 0.6f) + MU (0, MutSuper, 0.8f) + W (1, Puls, 0.6f, 0, 0.6f, -1) + MU (1, MutSuper, 0.6f)
             + F (FLp12, 3200, 0.15f) + E (2, 0.001f, 0.18f, 0.0f, 0.1f) + M (0, SrcEnv3, DstPitch, -0.12f) + L (0, Sin, false, 5.0f) + M (1, SrcLfo1, DstPitch, 0.006f)
             + E (1, 0.01f, 0.5f, 0.9f, 0.3f) + V { { Chorus, 0.6f }, { Binaural, 0.8f } },
             OP (0, O1Pos, 0.8f) + MU (0, MutSuper, 1.0f) + V { { Heat, 0.4f }, { Cut1, 5000 } });
        add ("RAVE", "Ravestab Ugn",
             W (0, Grund, saw, 0, 0.6f) + MU (0, MutSuper, 0.5f) + W (1, Grund, saw, 0, 0.5f, 0, 7) + MU (1, MutSuper, 0.5f) + W (2, Grund, saw, 0, 0.4f, 0, 3)
             + F (FLadder, 1500, 0.3f) + E (0, 0.001f, 0.25f, 0.0f, 0.2f) + E (1, 0.001f, 0.3f, 0.0f, 0.25f)
             + V { { FEnv, 0.55f }, { RumMix, 0.3f }, { Shimmer, 0.2f } },
             V { { FEnv, 0.8f }, { Cut1, 2500 }, { Heat, 0.3f } });
        add ("RAVE", "Synkpump",
             mono + W (0, Grund, saw, 0, 0.8f) + MU (0, MutSync, 0.15f) + A (2, square, 0, 0.4f, -1) + F (FLadder, 3500, 0.25f)
             + E (2, 0.001f, 0.35f, 0.0f, 0.2f) + M (0, SrcEnv3, DstMut1, 0.6f) + E (1, 0.002f, 0.4f, 0.9f, 0.2f),
             MAmt (0, 0.85f) + V { { Heat, 0.5f }, { Binaural, 0.9f } });
        add ("RAVE", "Fasdiff Stab",
             W (0, Grund, 0.9f, 0, 0.7f) + MU (0, MutPhase, 0.3f) + W (1, Grund, 0.9f, 0, 0.5f, 0, 12) + MU (1, MutPhase, 0.6f)
             + F (FLadder, 2000, 0.3f) + E (0, 0.001f, 0.3f, 0.0f, 0.2f) + E (1, 0.001f, 0.35f, 0.0f, 0.3f)
             + V { { FEnv, 0.5f }, { Shimmer, 0.3f }, { RumMix, 0.3f } },
             MU (0, MutPhase, 0.9f) + OP (0, O1Pos, 0.5f) + V { { Cut1, 3500 } });
        add ("RAVE", "Ravepiano Glöd",
             W (0, Orgel, 0.6f, 0, 0.8f) + MU (0, MutSuper, 0.3f) + F (FLp12, 5000, 0.1f) + E (1, 0.002f, 0.4f, 0.0f, 0.25f)
             + V { { FEnv, 0.0f }, { Chorus, 0.5f }, { RumMix, 0.25f } },
             OP (0, O1Pos, 1.0f) + V { { Chorus, 0.9f }, { Heat, 0.3f } });
        add ("RAVE", "Glödsåg",
             mono + W (0, Grund, saw, 0, 0.8f, -1) + MU (0, MutSuper, 0.35f) + W (1, Grund, square, 0, 0.5f, -2)
             + F (FLadder, 900, 0.3f) + E (0, 0.002f, 0.3f, 0.2f, 0.15f) + E (1, 0.002f, 0.4f, 1.0f, 0.12f)
             + V { { Heat, 0.35f }, { FEnv, 0.4f }, { RumMix, 0.0f } },
             V { { Heat, 0.75f }, { Cut1, 1800 } } + MU (0, MutSuper, 0.7f));
        add ("RAVE", "Kvintregn",
             W (0, Kvint, 0.5f, 0, 0.75f) + MU (0, MutSuper, 0.5f) + F (FLadder, 2200, 0.25f)
             + E (0, 0.002f, 0.6f, 0.2f, 0.3f) + E (1, 0.005f, 0.6f, 0.7f, 0.5f)
             + V { { FEnv, 0.4f }, { Shimmer, 0.35f }, { RumMix, 0.35f } },
             OP (0, O1Pos, 1.0f) + V { { Cut1, 4000 } });

        //======================================================================  YTA
        const V pad = E (1, 1.0f, 1.0f, 0.9f, 2.5f) + V { { FEnv, 0.15f }, { Spread, 0.6f }, { Binaural, 0.8f }, { RumMix, 0.4f }, { Width, 0.4f } };
        add ("YTA", "Smältvärme",
             pad + W (0, Grund, 0.5f, 0, 0.7f) + MU (0, MutSuper, 0.4f) + W (1, Grund, saw, 0, 0.5f, 0, 0, 8)
             + F (FLadder, 1800, 0.2f) + L (1, Smooth, false, 0.15f) + M (0, SrcLfo2, DstPosAll, 0.2f) + V { { Chorus, 0.5f } },
             V { { Cut1, 4000 }, { Shimmer, 0.5f } } + MU (0, MutSuper, 0.8f));
        add ("YTA", "Vokalkör",
             pad + W (0, Vokal, 0.3f, 0, 0.7f) + W (1, Vokal, 0.5f, 0, 0.6f, 0, 0, 8) + F (FLadder, 2600, 0.15f)
             + L (0, Sin, false, 0.2f) + L (1, Smooth, false, 0.13f) + M (0, SrcLfo1, DstPos1, 0.3f) + M (1, SrcLfo2, DstPos2, 0.3f)
             + V { { Chorus, 0.6f } },
             OP (0, O1Pos, 0.8f) + OP (1, O1Pos, 0.85f) + V { { Shimmer, 0.4f } });
        add ("YTA", "Glödande Glas",
             pad + W (0, Glas, 0.3f, 0, 0.7f) + MU (0, MutPhase, 0.3f) + W (1, Glas, 0.6f, 0, 0.4f, 0, 12)
             + F (FLp12, 5000, 0.1f) + L (1, Smooth, false, 0.2f) + M (0, SrcLfo2, DstPos1, 0.3f) + V { { Shimmer, 0.4f } },
             OP (0, O1Pos, 0.9f) + MU (0, MutPhase, 0.8f));
        add ("YTA", "Orgeldimma",
             pad + W (0, Orgel, 0.7f, 0, 0.7f) + W (1, Orgel, 0.4f, 0, 0.5f, 0, 12, 6) + F (FLp12, 3000, 0.1f)
             + E (1, 0.6f, 1.0f, 0.9f, 2.0f) + V { { Chorus, 0.7f } },
             V { { Heat, 0.3f }, { Cut1, 6000 } });
        add ("YTA", "Spektral Andning",
             pad + W (0, Spektrum, 0.15f, 0, 0.8f) + MU (0, MutSuper, 0.5f) + W (1, Grund, sine, 0, 0.3f, -1) + F (FLadder, 2500, 0.15f)
             + E (2, 4.0f, 2.0f, 1.0f, 2.0f) + M (0, SrcEnv3, DstPos1, 0.7f) + E (1, 1.2f, 1.0f, 0.9f, 3.0f),
             OP (0, O1Pos, 0.85f) + MAmt (0, -0.6f));
        add ("YTA", "Nattskiftet",
             pad + W (0, Puls, 0.3f, 0, 0.6f) + MU (0, MutSuper, 0.3f) + W (1, Puls, 0.6f, 0, 0.6f, 0, 0, -8)
             + F (FLadder, 1200, 0.3f) + L (0, Sin, false, 0.15f) + M (0, SrcLfo1, DstPosAll, 0.3f),
             V { { Heat, 0.4f }, { Cut1, 2500 } });
        add ("YTA", "Resonansmatta",
             pad + RS (0, 0.3f, 1.0f, 0.9f) + RS (1, 0.9f, 0.95f, 0.5f, 0, 7) + F (FLp12, 5000, 0.0f) + E (1, 1.0f, 1.0f, 0.9f, 3.0f),
             OP (0, O1Pos, 0.8f) + OP (1, O1Pos, 0.2f) + V { { Shimmer, 0.4f } });
        add ("YTA", "Varvsgryning",
             pad + W (0, Kvint, 0.3f, 0, 0.7f) + MU (0, MutSuper, 0.3f) + W (1, Grund, sine, 0, 0.4f, 1) + F (FLadder, 600, 0.2f)
             + E (2, 6.0f, 2.0f, 1.0f, 3.0f) + M (0, SrcEnv3, DstCutBoth, 0.35f) + V { { Shimmer, 0.5f } },
             V { { Cut1, 1500 }, { Shimmer, 0.8f } });

        //======================================================================  MOLN
        const V cloud = E (1, 1.2f, 1.0f, 0.9f, 3.0f) + V { { FEnv, 0.0f }, { Binaural, 0.9f }, { RumMix, 0.5f }, { Width, 0.4f }, { Spread, 0.6f } };
        add ("MOLN", "Partikelmoln",
             cloud + PT (0, Spektrum, 0.4f, 0.5f, 0.9f) + PT (1, Spektrum, 0.6f, 0.4f, 0.4f, 0, 12) + F (FLp12, 4000, 0.1f),
             OP (0, O1Form, 0.9f) + OP (0, O1Pos, 0.8f));
        add ("MOLN", "Ånghall",
             cloud + PT (0, Spektrum, 0.6f, 0.9f, 0.9f) + V { { Noise, 0.05f } } + F (FBand, 1200, 0.4f, FLp12, 3000, 0.1f)
             + L (0, Smooth, false, 0.1f) + M (0, SrcLfo1, DstCut1, 0.2f),
             V { { Shimmer, 0.6f }, { Cut1, 2500 } });
        add ("MOLN", "Gnistregn",
             cloud + E (1, 0.005f, 0.5f, 0.9f, 0.4f) + PT (0, Metall, 0.5f, 0.8f, 0.9f) + F (FLp12, 5000, 0.1f)
             + L (0, Rnd, true, d16) + M (0, SrcLfo1, DstPos1, 0.5f) + V { { Shimmer, 0.3f } },
             OP (0, O1Form, 0.3f) + V { { Heat, 0.3f } });
        add ("MOLN", "Glasdamm",
             cloud + PT (0, Glas, 0.5f, 0.7f, 0.9f) + F (FLp12, 6000, 0.1f) + L (1, Smooth, false, 0.4f) + M (0, SrcLfo2, DstPos1, 0.45f)
             + V { { Shimmer, 0.4f } },
             OP (0, O1Pos, 0.9f) + OP (0, O1Form, 0.95f));
        add ("MOLN", "Vokalsvärm",
             cloud + PT (0, Vokal, 0.3f, 0.6f, 0.9f) + PT (1, Vokal, 0.7f, 0.6f, 0.5f, 0, 7) + F (FLadder, 3000, 0.15f)
             + L (0, Tri, false, 0.1f) + M (0, SrcLfo1, DstPosAll, 0.3f),
             OP (0, O1Form, 0.95f) + V { { Binaural, 1.0f } });
        add ("MOLN", "Kornig Orgel",
             cloud + PT (0, Orgel, 0.6f, 0.4f, 0.9f) + W (1, Orgel, 0.6f, 0, 0.3f) + F (FLp12, 5000, 0.1f) + V { { Chorus, 0.5f } },
             OP (0, O1Form, 0.9f) + OP (1, O1Pos, 1.0f));
        add ("MOLN", "Slagghög",
             cloud + E (1, 0.05f, 0.8f, 0.7f, 1.0f) + PT (0, Metall, 0.5f, 0.9f, 0.9f, -1) + V { { Noise, 0.1f }, { FEnv, 0.3f }, { Heat, 0.3f } }
             + F (FLadder, 500, 0.5f),
             V { { Heat, 0.7f }, { Cut1, 1200 } });
        add ("MOLN", "Trappmoln",
             cloud + E (1, 0.3f, 1.0f, 0.9f, 1.5f) + PT (0, Trappa, 0.4f, 0.5f, 0.9f) + F (FLp12, 5000, 0.1f)
             + L (0, Rnd, true, d8) + M (0, SrcLfo1, DstPos1, 0.4f),
             OP (0, O1Pos, 0.9f) + V { { Shimmer, 0.4f } });

        //======================================================================  KLANG
        const V bell = V { { FEnv, 0.0f }, { FKey, 0.0f }, { RumMix, 0.3f } };
        add ("KLANG", "Stålklocka",
             bell + RS (0, 0.95f, 0.6f, 1.0f) + F (FLp12, 8000, 0.0f) + E (1, 0.001f, 3.0f, 0.0f, 2.0f),
             OP (0, O1Pos, 0.5f) + V { { Shimmer, 0.5f } });
        add ("KLANG", "Marimba Hall",
             bell + RS (0, 0.5f, 0.3f, 1.0f) + F (FLp12, 5000, 0.0f) + E (1, 0.001f, 1.2f, 0.0f, 0.8f),
             OP (0, O1Form, 0.7f));
        add ("KLANG", "Knäppt Sträng",
             bell + RS (0, 0.0f, 0.5f, 1.0f) + F (FLp12, 6000, 0.0f) + E (1, 0.001f, 2.0f, 0.0f, 1.0f) + V { { Spread, 0.6f } },
             OP (0, O1Pos, 0.3f));
        add ("KLANG", "FM Piano",
             bell + K (0, 0.45f, r1, 0.9f) + K (1, 0.2f, r7, 0.25f) + F (FLp12, 7000, 0.0f)
             + E (2, 0.001f, 1.0f, 0.0f, 0.5f) + M (0, SrcEnv3, DstPos1, 0.3f) + M (1, SrcVel, DstPos1, 0.2f)
             + E (1, 0.001f, 2.5f, 0.0f, 0.5f) + V { { Chorus, 0.25f }, { RumMix, 0.2f } },
             OP (0, O1Pos, 0.7f));
        add ("KLANG", "DX Klocka",
             bell + K (0, 0.55f, r35, 0.9f) + F (FLp12, 9000, 0.0f) + E (2, 0.001f, 2.0f, 0.1f, 1.0f)
             + M (0, SrcEnv3, DstPos1, 0.3f) + E (1, 0.001f, 3.0f, 0.0f, 1.5f),
             OP (0, O1Form, r7));
        add ("KLANG", "Glasplock",
             W (0, Glas, 0.6f, 0, 0.9f) + MU (0, MutPhase, 0.3f) + F (FLp12, 3000, 0.1f) + E (0, 0.001f, 0.3f, 0.0f, 0.3f)
             + E (1, 0.001f, 0.45f, 0.0f, 0.4f) + V { { FEnv, 0.4f }, { Shimmer, 0.35f }, { RumMix, 0.35f } },
             MU (0, MutPhase, 0.8f) + OP (0, O1Pos, 0.2f));
        add ("KLANG", "Kalimba",
             bell + RS (0, 0.6f, 0.25f, 1.0f) + K (1, 0.2f, r1, 0.3f) + F (FLp12, 6000, 0.0f) + E (1, 0.001f, 0.6f, 0.0f, 0.4f) + V { { RumMix, 0.2f } },
             OP (0, O1Pos, 0.9f));
        add ("KLANG", "Smidesstäd",
             bell + RS (0, 1.0f, 0.45f, 1.0f) + K (1, 0.6f, r141, 0.3f) + F (FLp12, 7000, 0.0f) + E (1, 0.001f, 1.5f, 0.0f, 1.0f)
             + V { { Heat, 0.2f } },
             V { { Heat, 0.6f } });

        //======================================================================  DRÖN
        add ("DRÖN", "Masugnen",
             W (0, Veck, 0.5f, 0, 0.7f, -1) + MU (0, MutFold, 0.3f) + RS (1, 0.8f, 0.9f, 0.4f, -1) + F (FLadder, 600, 0.3f)
             + E (1, 0.3f, 1.0f, 0.9f, 2.0f) + L (1, Smooth, false, 0.2f) + M (0, SrcLfo2, DstPos1, 0.3f)
             + V { { RumMix, 0.4f }, { Heat, 0.3f }, { FEnv, 0.15f } },
             V { { Heat, 0.8f } } + MU (0, MutFold, 0.7f));
        add ("DRÖN", "Kärnklyvning",
             K (0, 0.7f, r141, 0.9f) + F (FLp12, 3000, 0.1f) + L (1, Smooth, false, 0.3f) + M (0, SrcLfo2, DstPos1, 0.3f)
             + E (1, 0.01f, 1.0f, 0.7f, 0.8f) + V { { RumMix, 0.3f } },
             OP (0, O1Pos, 1.0f) + V { { Shimmer, 0.5f } });
        add ("DRÖN", "Svart Glas",
             W (0, Glas, 0.8f, 0, 0.8f, -1) + F (FNotch, 800, 0.5f, FLadder, 2000, 0.2f) + L (0, Sin, false, 0.1f)
             + M (0, SrcLfo1, DstCut1, 0.4f) + E (1, 0.5f, 1.0f, 0.9f, 2.0f) + V { { RumMix, 0.4f } },
             V { { Binaural, 1.0f }, { Width, 0.8f } });
        add ("DRÖN", "Tung Klocka",
             RS (0, 1.0f, 0.9f, 1.0f, -2) + F (FLp12, 6000, 0.0f) + E (1, 0.001f, 4.0f, 0.0f, 3.0f)
             + V { { FEnv, 0.0f }, { Drive, 0.3f }, { RumMix, 0.4f } },
             OP (0, O1Pos, 0.3f));
        add ("DRÖN", "Gruvgång",
             PT (0, Spektrum, 0.8f, 0.2f, 0.5f, -1) + V { { Noise, 0.4f } } + F (FBand, 400, 0.6f)
             + L (0, Smooth, false, 0.12f) + M (0, SrcLfo1, DstCut1, 0.3f) + E (1, 0.4f, 1.0f, 0.9f, 2.0f) + V { { RumMix, 0.45f } },
             V { { Cut1, 1200 } });
        add ("DRÖN", "Ångvissla",
             W (0, CZ, 0.8f, 0, 0.8f) + F (FComb, 400, 0.6f, FLp12, 4000, 0.1f) + E (1, 0.8f, 1.0f, 0.9f, 2.0f) + V { { RumMix, 0.4f } },
             V { { Cut1, 900 } });
        add ("DRÖN", "Smältdegel",
             mono + A (0, 0, 0, 0.6f) + A (1, 0, 0, 0.6f, 0, 0, -20) + A (2, 1, 0, 0.5f, -1) + F (FLadder, 400, 0.4f)
             + L (0, Tri, false, 0.2f) + M (0, SrcLfo1, DstCut1, 0.3f) + E (1, 0.01f, 0.5f, 1, 0.2f)
             + V { { Heat, 0.4f }, { Drive, 0.3f }, { RumMix, 0.15f } },
             V { { Heat, 0.8f } });
        add ("DRÖN", "Ekolod",
             RS (0, 0.4f, 0.7f, 1.0f) + M (0, SrcRandom, DstPos1, 0.3f) + F (FLp12, 5000, 0.0f) + E (1, 0.001f, 2.0f, 0.0f, 1.5f)
             + V { { FEnv, 0.0f }, { Shimmer, 0.5f }, { RumMix, 0.45f } },
             V { { Shimmer, 0.8f } });

        //======================================================================  LEAD
        add ("LEAD", "Glödtråd",
             bound + V { { Glide, 0.06f } } + W (0, Grund, saw, 0, 0.8f) + MU (0, MutSuper, 0.25f) + W (1, Grund, saw, 0, 0.4f, 0, 7)
             + F (FLadder, 2400, 0.3f) + E (0, 0.002f, 0.4f, 0.4f, 0.2f) + E (1, 0.005f, 0.5f, 0.9f, 0.2f)
             + L (0, Sin, false, 5.0f) + M (0, SrcLfo1, DstPitch, 0.008f) + M (1, SrcWheel, DstMut1, 0.6f),
             V { { Heat, 0.4f } });
        add ("LEAD", "Synksvep",
             mono + W (0, Synk, 0.15f, 0, 0.85f) + MU (0, MutSync, 0.2f) + F (FLadder, 4000, 0.2f) + E (2, 0.01f, 0.8f, 0.2f, 0.3f)
             + M (0, SrcEnv3, DstMut1, 0.6f) + E (1, 0.003f, 0.5f, 0.85f, 0.2f),
             MAmt (0, 0.9f) + V { { RumMix, 0.35f } });
        add ("LEAD", "Glaspipa",
             mono + V { { Glide, 0.04f } } + W (0, Glas, 0.4f, 0, 0.9f) + MU (0, MutPhase, 0.0f) + F (FLp12, 6000, 0.1f)
             + L (1, Smooth, false, 0.7f) + M (0, SrcLfo2, DstPos1, 0.2f) + E (1, 0.01f, 0.5f, 0.9f, 0.35f) + V { { RumMix, 0.25f } },
             OP (0, O1MAmt, 0.7f));
        add ("LEAD", "CZ Signal",
             mono + W (0, CZ, 0.3f, 0, 0.85f) + F (FLp12, 5000, 0.1f) + E (0, 0.002f, 0.5f, 0.1f, 0.2f) + M (0, SrcFEnv, DstPos1, 0.5f)
             + E (1, 0.003f, 0.5f, 0.8f, 0.2f) + V { { FEnv, 0.0f } },
             OP (0, O1Pos, 0.8f));
        add ("LEAD", "Kärnlead",
             mono + K (0, 0.45f, r2, 0.9f) + F (FLp12, 4500, 0.1f) + E (2, 0.002f, 0.6f, 0.3f, 0.3f)
             + M (0, SrcEnv3, DstPos1, 0.3f) + E (1, 0.003f, 0.5f, 0.85f, 0.25f),
             OP (0, O1Pos, 0.8f));
        add ("LEAD", "Fasdiff Lead",
             mono + W (0, Grund, saw, 0, 0.85f) + MU (0, MutPhase, 0.2f) + F (FLadder, 3000, 0.2f)
             + L (0, Tri, true, d2) + M (0, SrcLfo1, DstMut1, 0.3f) + E (1, 0.003f, 0.5f, 0.85f, 0.25f),
             V { { Heat, 0.5f } });
        add ("LEAD", "Vokallead",
             mono + W (0, Vokal, 0.4f, 0, 0.9f) + F (FLadder, 3000, 0.2f) + L (0, Tri, true, d4)
             + M (0, SrcLfo1, DstPos1, 0.45f) + E (1, 0.01f, 0.5f, 0.9f, 0.25f) + V { { Chorus, 0.3f } },
             OP (0, O1Pos, 0.8f));
        add ("LEAD", "Hetlead",
             mono + W (0, Grund, square, 0, 0.85f) + MU (0, MutFold, 0.4f) + F (FLadder, 1800, 0.45f)
             + E (0, 0.002f, 0.4f, 0.3f, 0.2f) + E (1, 0.003f, 0.5f, 0.9f, 0.2f) + V { { Heat, 0.5f }, { FEnv, 0.5f } },
             V { { Heat, 0.9f } } + MU (0, MutFold, 0.8f));
        return l;
    }();
    return list;
}

// Ett presets alla värden: grundläget + ändringarna + uppmätt volym.
inline std::array<float, NumParams> presetValues (int index)
{
    auto v = defaultValues();
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return v;
    for (const auto& [k, val] : list[(size_t) index].values) v[(size_t) k] = val;
    const auto& trims = presetTrims();
    if (juce::isPositiveAndBelow (index, (int) trims.size())) v[Vol] = juce::jlimit (-24.0f, 12.0f, v[Vol] + trims[(size_t) index]);
    return v;
}

// Ljud B: ljud A med B-ändringarna ovanpå. Volymen är densamma.
inline std::array<float, NumParams> presetValuesB (int index)
{
    auto v = presetValues (index);
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return v;
    for (const auto& [k, val] : list[(size_t) index].valuesB)
        if (k != Vol && k != Smalt) v[(size_t) k] = val;
    return v;
}
} // namespace masugn
