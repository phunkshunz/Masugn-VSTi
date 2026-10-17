#pragma once

#include "ParamTable.h"
#include <vector>

// MASUGN: ljudbiblioteket, GJUTFORMARNA. Sex grupper. Varje ljud är ändringar från grundläget.
// Volymen (VOL) är uppmätt med tools/MasugnTest så att alla låter ungefär lika starkt.
namespace masugn
{
struct Preset
{
    juce::String group, name;
    std::vector<std::pair<P, float>> values;
};

inline const juce::StringArray& presetGroups()
{
    static const juce::StringArray g { "BAS", "LEAD", "KLANG", "YTA", juce::String::fromUTF8 ("RÖRELSE"), "DJUP" };
    return g;
}

inline juce::String groupInfo (const juce::String& group)
{
    if (group == "BAS")   return juce::String::fromUTF8 ("Tunga basar ur tabeller och FM");
    if (group == "LEAD")  return juce::String::fromUTF8 ("Leads, svep och signaler");
    if (group == "KLANG") return juce::String::fromUTF8 ("Klockor, plock, stabs och klaviatur");
    if (group == "YTA")   return juce::String::fromUTF8 ("Mattor, moln och långa ytor");
    if (group == juce::String::fromUTF8 ("RÖRELSE")) return juce::String::fromUTF8 ("Tabeller som rör sig i takt");
    if (group == "DJUP")  return juce::String::fromUTF8 ("Udda, mörka och tunga ljud");
    return {};
}

// Uppmätta volymer (dB), samma ordning som listan nedan. Fylls i av tools/MasugnTest.
inline const std::vector<float>& presetTrims()
{
    static const std::vector<float> t {
        // MASUGN_TRIMS_BEGIN
        0.0f, // Smält Bly
        1.5f, // Gjutjärn
        -5.0f, // FM Kärnbas
        2.1f, // Vokalbas
        -0.5f, // Reese Ugn
        -1.9f, // Slaggbas
        -1.5f, // Trappbas
        4.1f, // Puls 303
        7.4f, // Kamrör
        1.7f, // Glödtråd
        2.8f, // Synksvep
        -1.7f, // Glaspipa
        1.7f, // CZ Signal
        -4.2f, // Kärnlead
        5.2f, // Metallpipa
        6.3f, // Vokallead
        3.3f, // Kvintlead
        -0.7f, // Hoover Ugn
        3.1f, // Gnistlead
        -8.9f, // Stålklocka
        -5.7f, // Marimba Hall
        -7.1f, // Knäppt Sträng
        -0.3f, // FM Piano
        -7.2f, // DX Klocka
        3.4f, // Glasplock
        -3.4f, // Orgel Maskinhall
        -7.0f, // Kalimba
        5.7f, // Detroit Stab
        4.7f, // Smältvärme
        6.2f, // Partikelmoln
        7.7f, // Vokalkör
        4.1f, // Glashall
        7.9f, // Ånghall
        -2.2f, // Orgeldimma
        -7.9f, // Resonansmatta
        2.3f, // Spektral Dimma
        1.1f, // Nattskiftet
        4.6f, // Vågsvep
        4.0f, // Pulsande Ugn
        2.6f, // Slumpgnistor
        -0.0f, // Trappsteg
        5.3f, // Formantgång
        2.9f, // CZ Rullband
        0.6f, // Kamsvep
        0.5f, // Glasregn
        -3.9f, // Kärnpuls
        -3.4f, // Masugnen
        1.9f, // Slagghög
        -2.0f, // Kärnklyvning
        12.6f, // Svart Glas
        -11.4f, // Tung Klocka
        12.1f, // Gruvgång
        4.1f, // Ångvissla
        -2.8f, // Smältdegel
        -8.7f, // Ekolod
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

// taktdelar
constexpr float d4bars = 0, d2bars = 1, d1bar = 2, d2 = 3, d4 = 4, d8 = 5, d16 = 6;
// tabeller
enum { Grund, Puls, Synk, Veck, Vokal, Trappa, Glas, Spektrum, Metall, Orgel, Kvint, CZ };
// FM-kvoter (form-värden för kärnan)
constexpr float r05 = 0.0f, r1 = 0.09f, r141 = 0.18f, r2 = 0.27f, r3 = 0.36f, r35 = 0.45f, r4 = 0.55f, r5 = 0.64f, r7 = 0.82f;
} // namespace build

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list = []
    {
        using namespace build;
        std::vector<Preset> l;
        auto add = [&l] (const char* g, const char* n, V v) { l.push_back ({ juce::String::fromUTF8 (g), juce::String::fromUTF8 (n), std::move (v) }); };
        const V mono  { { Mode, 1 }, { Spread, 0 } };
        const V bound { { Mode, 2 }, { Spread, 0 } };

        //======================================================================  BAS
        add ("BAS", "Smält Bly", mono + W (0, Grund, 0.68f, 0, 0.8f, -1) + A (2, 1, 0, 0.5f, -2) + F (FLadder, 260, 0.35f)
                                 + V { { FEnv, 0.45f }, { FKey, 0.2f } } + E (0, 0.002f, 0.25f, 0.1f, 0.1f) + E (1, 0.002f, 0.3f, 1, 0.08f));
        add ("BAS", "Gjutjärn",  mono + W (0, Synk, 0.35f, 0, 0.8f, -1) + A (2, 1, 0, 0.4f, -2) + F (FLadder, 380, 0.3f)
                                 + E (0, 0.002f, 0.3f, 0.15f, 0.1f) + E (1, 0.002f, 0.4f, 1, 0.08f) + M (0, SrcFEnv, DstPos1, 0.35f));
        add ("BAS", "FM Kärnbas", mono + K (0, 0.55f, r1, 0.9f, -1) + A (2, 0, 0, 0.3f, -2) + F (FLp12, 1600, 0.1f)
                                 + V { { FEnv, 0.25f } } + E (2, 0.001f, 0.18f, 0.0f, 0.1f) + E (1, 0.002f, 0.5f, 1, 0.08f) + M (0, SrcEnv3, DstPos1, 0.35f));
        add ("BAS", "Vokalbas",  mono + W (0, Vokal, 0.2f, 0, 0.9f, -1) + A (2, 1, 0, 0.35f, -2) + F (FLadder, 700, 0.25f)
                                 + E (1, 0.002f, 0.4f, 1, 0.08f) + L (0, 1, true, d8) + M (0, SrcLfo1, DstPos1, 0.3f));
        add ("BAS", "Reese Ugn", mono + W (0, Grund, 0.67f, 0, 0.7f, -1) + W (1, Grund, 0.67f, 0, 0.7f, -1, 0, 14) + F (FLp12, 900, 0.2f)
                                 + V { { FEnv, 0.15f }, { Chorus, 0.25f } } + E (1, 0.005f, 0.4f, 1, 0.12f) + L (1, 0, false, 0.15f) + M (0, SrcLfo2, DstCut1, 0.12f));
        add ("BAS", "Slaggbas",  mono + W (0, Veck, 0.3f, 0, 0.8f, -1) + A (2, 1, 0, 0.35f, -2) + F (FLadder, 650, 0.3f)
                                 + V { { FEnv, 0.4f } } + E (0, 0.002f, 0.22f, 0.1f, 0.1f) + E (1, 0.002f, 0.35f, 1, 0.07f) + M (0, SrcVel, DstPos1, 0.4f));
        add ("BAS", "Trappbas",  mono + W (0, Trappa, 0.35f, 0, 0.8f, -1) + A (2, 1, 0, 0.3f, -2) + F (FLadder, 1100, 0.25f)
                                 + V { { FEnv, 0.3f } } + E (0, 0.002f, 0.2f, 0.2f, 0.1f) + E (1, 0.002f, 0.3f, 1, 0.07f));
        add ("BAS", "Puls 303",  bound + V { { Glide, 0.08f } } + A (0, 0.0f, 0.15f, 0.9f, -1) + F (FLadder, 420, 0.68f)
                                 + V { { FEnv, 0.65f }, { FDrive, 0.35f } } + E (0, 0.002f, 0.22f, 0.0f, 0.1f) + E (1, 0.002f, 0.3f, 1, 0.06f) + M (0, SrcVel, DstCut1, 0.15f));
        add ("BAS", "Kamrör",    mono + W (0, Grund, 0.67f, 0, 0.8f, -1) + F (FComb, 262, 0.55f, FLadder, 1400, 0.2f)
                                 + V { { FKey, 1.0f }, { FEnv, 0.2f } } + E (1, 0.002f, 0.4f, 1, 0.08f));

        //======================================================================  LEAD
        add ("LEAD", "Glödtråd",  bound + V { { Glide, 0.06f } } + W (0, Grund, 0.67f, 0, 0.8f) + W (1, Grund, 0.67f, 0, 0.4f, 0, 7) + F (FLadder, 2400, 0.3f)
                                  + E (0, 0.002f, 0.4f, 0.4f, 0.2f) + E (1, 0.005f, 0.5f, 0.9f, 0.2f) + L (0, 0, false, 5.0f) + M (0, SrcLfo1, DstPitch, 0.008f)
                                  + M (1, SrcWheel, DstPitch, 0.0f));
        add ("LEAD", "Synksvep",  mono + W (0, Synk, 0.15f, 0, 0.85f) + F (FLadder, 4000, 0.2f) + E (2, 0.01f, 0.8f, 0.2f, 0.3f)
                                  + E (1, 0.003f, 0.5f, 0.85f, 0.2f) + M (0, SrcEnv3, DstPos1, 0.6f) + V { { EkoMix, 0.2f }, { EkoDiv, 2 } });
        add ("LEAD", "Glaspipa",  mono + V { { Glide, 0.04f } } + W (0, Glas, 0.4f, 0, 0.9f) + F (FLp12, 6000, 0.1f) + L (1, 5, false, 0.7f)
                                  + M (0, SrcLfo2, DstPos1, 0.2f) + E (1, 0.01f, 0.5f, 0.9f, 0.35f) + V { { RumMix, 0.25f } });
        add ("LEAD", "CZ Signal", mono + W (0, CZ, 0.3f, 0, 0.85f) + F (FLp12, 5000, 0.1f) + V { { FEnv, 0.0f } }
                                  + E (0, 0.002f, 0.5f, 0.1f, 0.2f) + M (0, SrcFEnv, DstPos1, 0.5f) + E (1, 0.003f, 0.5f, 0.8f, 0.2f) + V { { EkoMix, 0.18f }, { EkoDiv, 1 } });
        add ("LEAD", "Kärnlead",  mono + K (0, 0.45f, r2, 0.9f) + F (FLp12, 4500, 0.1f) + E (2, 0.002f, 0.6f, 0.3f, 0.3f)
                                  + M (0, SrcEnv3, DstPos1, 0.3f) + E (1, 0.003f, 0.5f, 0.85f, 0.25f) + V { { EkoMix, 0.2f }, { EkoDiv, 2 } });
        add ("LEAD", "Metallpipa",mono + V { { Glide, 0.05f } } + W (0, Metall, 0.3f, 0, 0.8f) + F (FLadder, 3000, 0.25f)
                                  + E (0, 0.002f, 0.3f, 0.3f, 0.2f) + E (1, 0.003f, 0.4f, 0.8f, 0.2f));
        add ("LEAD", "Vokallead", mono + W (0, Vokal, 0.4f, 0, 0.9f) + F (FLadder, 3000, 0.2f) + L (0, 1, true, d4)
                                  + M (0, SrcLfo1, DstPos1, 0.45f) + E (1, 0.01f, 0.5f, 0.9f, 0.25f) + V { { Chorus, 0.3f } });
        add ("LEAD", "Kvintlead", bound + V { { Glide, 0.07f } } + W (0, Kvint, 0.6f, 0, 0.85f) + F (FLadder, 2800, 0.25f)
                                  + E (0, 0.002f, 0.4f, 0.3f, 0.2f) + E (1, 0.003f, 0.5f, 0.9f, 0.2f));
        add ("LEAD", "Hoover Ugn",mono + V { { Glide, 0.15f } } + W (0, Puls, 0.5f, 0, 0.6f) + W (1, Puls, 0.5f, 0, 0.6f, 0, 0, 15) + A (2, 0, 0, 0.5f, -1)
                                  + F (FLp12, 3500, 0.15f) + L (0, 1, false, 4.5f) + M (0, SrcLfo1, DstPosAll, 0.25f)
                                  + E (1, 0.01f, 0.5f, 0.9f, 0.3f) + V { { Chorus, 0.5f } });
        add ("LEAD", "Gnistlead", mono + PT (0, Metall, 0.4f, 0.6f, 0.9f) + W (1, Grund, 0.67f, 0, 0.4f, -1) + F (FLadder, 3200, 0.2f)
                                  + E (1, 0.005f, 0.5f, 0.85f, 0.3f) + V { { EkoMix, 0.2f }, { EkoDiv, 1 } });

        //======================================================================  KLANG
        add ("KLANG", "Stålklocka",  RS (0, 0.95f, 0.6f, 1.0f) + F (FLp12, 8000, 0.0f) + V { { FEnv, 0.0f }, { FKey, 0.0f } }
                                     + E (1, 0.001f, 3.0f, 0.0f, 2.0f) + V { { RumMix, 0.3f } });
        add ("KLANG", "Marimba Hall", RS (0, 0.5f, 0.3f, 1.0f) + F (FLp12, 5000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.001f, 1.2f, 0.0f, 0.8f) + V { { RumMix, 0.25f } });
        add ("KLANG", "Knäppt Sträng",RS (0, 0.0f, 0.5f, 1.0f) + F (FLp12, 6000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.001f, 2.0f, 0.0f, 1.0f) + V { { RumMix, 0.2f }, { Spread, 0.6f } });
        add ("KLANG", "FM Piano",    K (0, 0.45f, r1, 0.9f) + K (1, 0.2f, r7, 0.25f) + F (FLp12, 7000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (2, 0.001f, 1.0f, 0.0f, 0.5f) + M (0, SrcEnv3, DstPos1, 0.3f) + M (1, SrcVel, DstPos1, 0.2f)
                                     + E (1, 0.001f, 2.5f, 0.0f, 0.5f) + V { { Chorus, 0.25f } });
        add ("KLANG", "DX Klocka",   K (0, 0.55f, r35, 0.9f) + F (FLp12, 9000, 0.0f) + V { { FEnv, 0.0f } } + E (2, 0.001f, 2.0f, 0.1f, 1.0f)
                                     + M (0, SrcEnv3, DstPos1, 0.3f) + E (1, 0.001f, 3.0f, 0.0f, 1.5f) + V { { RumMix, 0.3f } });
        add ("KLANG", "Glasplock",   W (0, Glas, 0.6f, 0, 0.9f) + F (FLp12, 3000, 0.1f) + V { { FEnv, 0.4f } } + E (0, 0.001f, 0.3f, 0.0f, 0.3f)
                                     + E (1, 0.001f, 0.45f, 0.0f, 0.4f) + V { { EkoMix, 0.25f }, { EkoDiv, 2 } });
        add ("KLANG", "Orgel Maskinhall", W (0, Orgel, 0.5f, 0, 0.9f) + F (FLp12, 8000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.005f, 0.2f, 1.0f, 0.12f) + V { { Chorus, 0.5f }, { RumMix, 0.2f } });
        add ("KLANG", "Kalimba",     RS (0, 0.6f, 0.25f, 1.0f) + K (1, 0.2f, r1, 0.3f) + F (FLp12, 6000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.001f, 0.6f, 0.0f, 0.4f) + V { { RumMix, 0.2f } });
        add ("KLANG", "Detroit Stab",W (0, Grund, 0.67f, 0, 0.6f) + W (1, Grund, 0.67f, 0, 0.5f, 0, 7) + W (2, Grund, 0.67f, 0, 0.4f, 0, 3)
                                     + F (FLadder, 1400, 0.3f) + V { { FEnv, 0.5f } } + E (0, 0.001f, 0.2f, 0.0f, 0.2f)
                                     + E (1, 0.001f, 0.25f, 0.0f, 0.2f) + V { { EkoMix, 0.25f }, { EkoDiv, 2 } });

        //======================================================================  YTA
        const V pad = E (1, 1.0f, 1.0f, 0.9f, 2.5f) + V { { FEnv, 0.15f }, { Spread, 0.7f } };
        add ("YTA", "Smältvärme",     pad + W (0, Grund, 0.5f, 0, 0.7f) + W (1, Grund, 0.5f, 0, 0.6f, 0, 0, 10) + F (FLadder, 1800, 0.2f)
                                      + L (1, 5, false, 0.15f) + M (0, SrcLfo2, DstPosAll, 0.2f) + V { { Chorus, 0.5f }, { RumMix, 0.4f } });
        add ("YTA", "Partikelmoln",   pad + PT (0, Spektrum, 0.4f, 0.5f, 0.9f) + PT (1, Spektrum, 0.6f, 0.4f, 0.4f, 0, 12) + F (FLp12, 4000, 0.1f)
                                      + E (1, 1.5f, 1.0f, 0.9f, 3.0f) + V { { RumMix, 0.5f } });
        add ("YTA", "Vokalkör",       pad + W (0, Vokal, 0.3f, 0, 0.7f) + W (1, Vokal, 0.5f, 0, 0.6f, 0, 0, 8) + F (FLadder, 2600, 0.15f)
                                      + L (0, 0, false, 0.2f) + L (1, 5, false, 0.13f) + M (0, SrcLfo1, DstPos1, 0.3f) + M (1, SrcLfo2, DstPos2, 0.3f)
                                      + V { { Chorus, 0.6f }, { RumMix, 0.4f } });
        add ("YTA", "Glashall",       pad + W (0, Glas, 0.3f, 0, 0.7f) + PT (1, Glas, 0.7f, 0.3f, 0.45f, 0, 12) + F (FLp12, 5000, 0.1f)
                                      + L (1, 5, false, 0.2f) + M (0, SrcLfo2, DstPos1, 0.3f) + V { { RumMix, 0.6f } });
        add ("YTA", "Ånghall",        pad + PT (0, Spektrum, 0.6f, 0.9f, 0.9f) + V { { Noise, 0.05f } } + F (FBand, 1200, 0.4f, FLp12, 3000, 0.1f)
                                      + L (0, 5, false, 0.1f) + M (0, SrcLfo1, DstCut1, 0.2f) + V { { RumMix, 0.55f } });
        add ("YTA", "Orgeldimma",     pad + W (0, Orgel, 0.7f, 0, 0.7f) + W (1, Orgel, 0.4f, 0, 0.5f, 0, 12, 6) + F (FLp12, 3000, 0.1f)
                                      + E (1, 0.6f, 1.0f, 0.9f, 2.0f) + V { { Chorus, 0.7f }, { RumMix, 0.4f } });
        add ("YTA", "Resonansmatta",  pad + RS (0, 0.3f, 1.0f, 0.9f) + RS (1, 0.9f, 0.95f, 0.5f, 0, 7) + F (FLp12, 5000, 0.0f)
                                      + E (1, 1.0f, 1.0f, 0.9f, 3.0f) + V { { RumMix, 0.5f } });
        add ("YTA", "Spektral Dimma", pad + W (0, Spektrum, 0.15f, 0, 0.8f) + W (1, Grund, 0.0f, 0, 0.3f, -1) + F (FLadder, 2500, 0.15f)
                                      + E (2, 4.0f, 2.0f, 1.0f, 2.0f) + M (0, SrcEnv3, DstPos1, 0.7f) + E (1, 1.2f, 1.0f, 0.9f, 3.0f) + V { { RumMix, 0.4f } });
        add ("YTA", "Nattskiftet",    pad + W (0, Puls, 0.3f, 0, 0.6f) + W (1, Puls, 0.6f, 0, 0.6f, 0, 0, -8) + F (FLadder, 1200, 0.3f)
                                      + L (0, 0, false, 0.15f) + M (0, SrcLfo1, DstPosAll, 0.3f) + V { { EkoMix, 0.2f }, { EkoDiv, 4 }, { RumMix, 0.4f } });

        //======================================================================  RÖRELSE
        const V hold = E (1, 0.005f, 0.5f, 0.9f, 0.4f) + V { { FEnv, 0.1f } };
        add ("RÖRELSE", "Vågsvep",       hold + W (0, Spektrum, 0.1f, 0, 0.85f) + F (FLadder, 2500, 0.2f) + L (0, 2, true, d1bar)
                                         + M (0, SrcLfo1, DstPos1, 0.45f));
        add ("RÖRELSE", "Pulsande Ugn",  hold + W (0, Synk, 0.3f, 0, 0.85f) + F (FLadder, 1100, 0.4f) + L (0, 3, true, d8)
                                         + M (0, SrcLfo1, DstCut1, 0.3f) + V { { EkoMix, 0.3f }, { EkoDiv, 2 } });
        add ("RÖRELSE", "Slumpgnistor",  hold + PT (0, Metall, 0.5f, 0.8f, 0.9f) + F (FLp12, 5000, 0.1f) + L (0, 4, true, d16)
                                         + M (0, SrcLfo1, DstPos1, 0.5f) + V { { EkoMix, 0.3f }, { EkoDiv, 1 } });
        add ("RÖRELSE", "Trappsteg",     hold + W (0, Trappa, 0.5f, 0, 0.85f) + F (FLadder, 2000, 0.3f) + L (0, 4, true, d8)
                                         + M (0, SrcLfo1, DstPos1, 0.4f));
        add ("RÖRELSE", "Formantgång",   hold + W (0, Vokal, 0.5f, 0, 0.9f) + F (FLadder, 2500, 0.2f) + L (0, 1, true, d1bar) + L (1, 0, true, d4)
                                         + M (0, SrcLfo1, DstPos1, 0.45f) + M (1, SrcLfo2, DstCut1, 0.15f));
        add ("RÖRELSE", "CZ Rullband",   hold + W (0, CZ, 0.4f, 0, 0.85f) + F (FLadder, 3000, 0.2f) + L (0, 2, true, d4)
                                         + M (0, SrcLfo1, DstPos1, 0.35f));
        add ("RÖRELSE", "Kamsvep",       hold + W (0, Grund, 0.67f, 0, 0.8f) + F (FComb, 200, 0.65f, FLp12, 4000, 0.1f) + V { { FKey, 0.0f } }
                                         + L (0, 1, true, d2bars) + M (0, SrcLfo1, DstCut1, 0.3f));
        add ("RÖRELSE", "Glasregn",      hold + PT (0, Glas, 0.5f, 0.7f, 0.9f) + F (FLp12, 6000, 0.1f) + L (1, 5, false, 0.4f)
                                         + M (0, SrcLfo2, DstPos1, 0.45f) + V { { RumMix, 0.5f }, { EkoMix, 0.3f }, { EkoDiv, 4 } });
        add ("RÖRELSE", "Kärnpuls",      hold + K (0, 0.3f, r2, 0.9f) + F (FLp12, 4000, 0.1f) + L (0, 0, true, d8)
                                         + M (0, SrcLfo1, DstPos1, 0.3f) + V { { EkoMix, 0.25f }, { EkoDiv, 2 } });

        //======================================================================  DJUP
        add ("DJUP", "Masugnen",     W (0, Veck, 0.5f, 0, 0.7f, -1) + RS (1, 0.8f, 0.9f, 0.4f, -1) + F (FLadder, 600, 0.3f)
                                     + E (1, 0.3f, 1.0f, 0.9f, 2.0f) + L (1, 5, false, 0.2f) + M (0, SrcLfo2, DstPos1, 0.3f)
                                     + V { { RumMix, 0.4f }, { Drive, 0.2f }, { FEnv, 0.15f } });
        add ("DJUP", "Slagghög",     PT (0, Metall, 0.5f, 0.9f, 0.9f, -1) + V { { Noise, 0.1f } } + F (FLadder, 500, 0.5f)
                                     + E (1, 0.05f, 0.8f, 0.7f, 1.0f) + V { { RumMix, 0.3f }, { FEnv, 0.3f } });
        add ("DJUP", "Kärnklyvning", K (0, 0.7f, r141, 0.9f) + F (FLp12, 3000, 0.1f) + L (1, 5, false, 0.3f) + M (0, SrcLfo2, DstPos1, 0.3f)
                                     + E (1, 0.01f, 1.0f, 0.7f, 0.8f) + V { { EkoMix, 0.3f }, { EkoDiv, 4 } });
        add ("DJUP", "Svart Glas",   W (0, Glas, 0.8f, 0, 0.8f, -1) + F (FNotch, 800, 0.5f, FLadder, 2000, 0.2f) + L (0, 0, false, 0.1f)
                                     + M (0, SrcLfo1, DstCut1, 0.4f) + E (1, 0.5f, 1.0f, 0.9f, 2.0f) + V { { RumMix, 0.4f } });
        add ("DJUP", "Tung Klocka",  RS (0, 1.0f, 0.9f, 1.0f, -2) + F (FLp12, 6000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.001f, 4.0f, 0.0f, 3.0f) + V { { Drive, 0.3f }, { RumMix, 0.4f } });
        add ("DJUP", "Gruvgång",     PT (0, Spektrum, 0.8f, 0.2f, 0.5f, -1) + V { { Noise, 0.4f } } + F (FBand, 400, 0.6f)
                                     + L (0, 5, false, 0.12f) + M (0, SrcLfo1, DstCut1, 0.3f) + E (1, 0.4f, 1.0f, 0.9f, 2.0f) + V { { RumMix, 0.45f } });
        add ("DJUP", "Ångvissla",    W (0, CZ, 0.8f, 0, 0.8f) + F (FComb, 400, 0.6f, FLp12, 4000, 0.1f) + E (1, 0.8f, 1.0f, 0.9f, 2.0f)
                                     + V { { RumMix, 0.4f } });
        add ("DJUP", "Smältdegel",   mono + A (0, 0, 0, 0.6f) + A (1, 0, 0, 0.6f, 0, 0, -20) + A (2, 1, 0, 0.5f, -1) + F (FLadder, 400, 0.4f)
                                     + L (0, 1, false, 0.2f) + M (0, SrcLfo1, DstCut1, 0.3f) + E (1, 0.01f, 0.5f, 1, 0.2f) + V { { Drive, 0.3f } });
        add ("DJUP", "Ekolod",       RS (0, 0.4f, 0.7f, 1.0f) + M (0, SrcRandom, DstPos1, 0.3f) + F (FLp12, 5000, 0.0f) + V { { FEnv, 0.0f } }
                                     + E (1, 0.001f, 2.0f, 0.0f, 1.5f) + V { { EkoMix, 0.45f }, { EkoFb, 0.55f }, { EkoTone, 0.3f }, { EkoDiv, 4 }, { RumMix, 0.3f } });
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
} // namespace masugn
