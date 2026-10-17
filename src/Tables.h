#pragma once

#include <juce_core/juce_core.h>
#include <complex>
#include <vector>
#include <cmath>

// MASUGN: vågtabellerna. Tolv egna tabeller med 32 vågor var, räknade fram när pluginet startar.
// Varje våg finns i nio versioner med färre och färre övertoner (mip-nivåer),
// så att höga toner aldrig viker ner fula övertoner (aliasing).
namespace masugn
{
constexpr int kTableSize = 1024;      // sampel per våg
constexpr int kFrames    = 32;        // vågor per tabell
constexpr int kMips      = 9;         // 512, 256, ... 2 övertoner
constexpr int kNumTables = 12;

inline const char* const* tableNames()
{
    static const char* n[kNumTables] = { "Grund", "Puls", "Synk", "Veck", "Vokal", "Trappa",
                                         "Glas", "Spektrum", "Metall", "Orgel", "Kvint", "CZ Reso" };
    return n;
}

namespace detail
{
// Enkel radix-2 FFT. Egen, så att tabellerna blir exakt likadana på Mac och Windows.
inline void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * juce::MathConstants<double>::pi / (double) len * (inverse ? 1.0 : -1.0);
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

inline double frac (double x) { return x - std::floor (x); }
inline double saw (double t) { return 2.0 * frac (t) - 1.0; }

// En våg i tidsdomänen: t = 0..1 genom vågen, x = 0..1 genom tabellen.
inline double shape (int table, double t, double x)
{
    const double pi = juce::MathConstants<double>::pi, tp = 2.0 * pi;
    switch (table)
    {
        case 0: // GRUND: sinus -> triangel -> såg -> fyrkant
        {
            const double si = std::sin (tp * t);
            const double tr = 1.0 - 4.0 * std::abs (frac (t + 0.25) - 0.5);
            const double sa = saw (t);
            const double sq = frac (t) < 0.5 ? 1.0 : -1.0;
            const double s = x * 3.0;
            if (s < 1.0) return si + (tr - si) * s;
            if (s < 2.0) return tr + (sa - tr) * (s - 1.0);
            return sa + (sq - sa) * (s - 2.0);
        }
        case 1: // PULS: pulsbredd från 50 % ned till smal nål
        {
            const double w = 0.5 - 0.47 * x;
            return frac (t) < w ? 1.0 : -1.0;
        }
        case 2: // SYNK: hårdsynkad såg som sveper uppåt
        {
            const double r = 1.0 + x * 7.0;
            return saw (t * r) * (0.6 + 0.4 * (1.0 - t));
        }
        case 3: // VECK: sinus som viks fler och fler gånger
        {
            const double g = 1.0 + x * 7.0;
            return std::sin (g * std::sin (tp * t) * pi * 0.5);
        }
        case 5: // TRAPPA: grovt kvantiserad sinus/såg, digitalt och 80-tal
        {
            const double steps = 3.0 + std::round (x * 13.0);
            const double base = x < 0.5 ? std::sin (tp * t) : saw (t);
            return std::round (base * steps) / steps;
        }
        case 8: // METALL: ringmodulerad såg, hård och klangig
        {
            const double k = 2.0 + std::round (x * 12.0);
            return saw (t) * (0.3 + 0.7 * std::sin (tp * k * t)) + 0.2 * std::sin (tp * t);
        }
        case 10: // KVINT: såg med oktav och kvint ovanpå
            return saw (t) + x * 0.8 * saw (2.0 * t) + x * x * 0.6 * saw (3.0 * t);
        case 11: // CZ RESO: avklingande resonans inuti varje våg, som Casio CZ
        {
            const double r = 1.0 + x * 15.0;
            const double win = 1.0 - frac (t);
            return std::sin (tp * r * frac (t)) * win;
        }
        default: return std::sin (tp * t);
    }
}

// De additiva tabellerna räknas direkt som övertoner (snabbt, ingen FFT behövs).
// Fyller spec[h] för h = 1..512 och svarar true, annars false (då används shape()).
inline bool harmonics (int table, double x, std::vector<std::complex<double>>& spec)
{
    auto put = [&spec] (int h, double a, double ph)
    {
        // sin(2 pi h t + ph) i FFT-form (bara förhållandet mellan övertonerna spelar roll)
        spec[(size_t) h] += std::complex<double> (a * std::sin (ph), -a * std::cos (ph));
    };
    switch (table)
    {
        case 4: // VOKAL: två formanter som vandrar, som en röst
        {
            const double f1 = 2.0 + x * 9.0, f2 = 6.0 + x * 22.0;
            for (int h = 1; h <= 64; ++h)
                put (h, (std::exp (-0.5 * std::pow ((h - f1) / 1.6, 2.0)) + 0.6 * std::exp (-0.5 * std::pow ((h - f2) / 3.0, 2.0)))
                        / std::pow ((double) h, 0.35) + 0.08 / h, 0.0);
            return true;
        }
        case 6: // GLAS: smala toppar av höga övertoner som vandrar uppåt
        {
            const double c = 2.0 + x * 40.0;
            put (1, 0.4, 0.0);
            for (int h = 2; h <= 96; ++h)
                if (h % 3 != 0) put (h, std::exp (-0.5 * std::pow ((h - c) / 2.2, 2.0)), h * 0.7);
            return true;
        }
        case 7: // SPEKTRUM: slumpade spektra (alltid samma) som glider in i varandra
        {
            const double pos = x * 5.0;
            const int i0 = juce::jmin (4, (int) pos);
            const double fr = pos - i0;
            for (int h = 1; h <= 80; ++h)
            {
                juce::Random r0 ((juce::int64) (i0 * 1000 + h)), r1 ((juce::int64) ((i0 + 1) * 1000 + h));
                const double a0 = r0.nextDouble() * r0.nextDouble(), a1 = r1.nextDouble() * r1.nextDouble();
                put (h, (a0 + (a1 - a0) * fr) / std::pow ((double) h, 0.9 - 0.4 * x), h * 1.3);
            }
            return true;
        }
        case 9: // ORGEL: dragstänger som drar ut efter hand
        {
            const int hs[] = { 1, 2, 3, 4, 6, 8, 10, 12 };
            for (int i = 0; i < 8; ++i)
                put (hs[i], i == 0 ? 1.0 : juce::jlimit (0.0, 1.0, x * 8.0 - i + 1.0) * (0.9 - 0.06 * i), 0.0);
            return true;
        }
        default: return false;
    }
}
} // namespace detail

class WaveBank
{
public:
    static const WaveBank& get()
    {
        static const WaveBank bank;
        return bank;
    }

    // Peka på en våg: tabell, mip-nivå, våg. Varje våg har ett extra sampel på slutet (för interpolering).
    const float* frame (int table, int mip, int f) const noexcept
    {
        return data.data() + (((size_t) table * kMips + (size_t) mip) * kFrames + (size_t) f) * (kTableSize + 1);
    }

    // Vilken mip-nivå som räcker för en ton: inga övertoner över halva samplingsfrekvensen.
    static int mipFor (float hz, float fs) noexcept
    {
        int m = 0;
        while (m < kMips - 1 && (float) (512 >> m) * hz > fs * 0.5f) ++m;
        return m;
    }

private:
    WaveBank()
    {
        data.assign ((size_t) kNumTables * kMips * kFrames * (kTableSize + 1), 0.0f);
        constexpr int big = 4096;
        std::vector<std::complex<double>> spec ((size_t) big), small ((size_t) kTableSize);

        for (int t = 0; t < kNumTables; ++t)
            for (int f = 0; f < kFrames; ++f)
            {
                const double x = (double) f / (double) (kFrames - 1);
                std::fill (spec.begin(), spec.end(), std::complex<double> (0.0, 0.0));
                if (! detail::harmonics (t, x, spec))
                {
                    for (int i = 0; i < big; ++i)
                        spec[(size_t) i] = detail::shape (t, (double) i / (double) big, x);
                    detail::fft (spec, false);
                }

                float gain = 1.0f;
                for (int m = 0; m < kMips; ++m)
                {
                    const int maxH = 512 >> m;
                    std::fill (small.begin(), small.end(), std::complex<double> (0.0, 0.0));
                    for (int h = 1; h <= maxH; ++h)
                    {
                        small[(size_t) h] = spec[(size_t) h];
                        small[(size_t) (kTableSize - h)] = std::conj (spec[(size_t) h]);
                    }
                    detail::fft (small, true);

                    float* out = const_cast<float*> (frame (t, m, f));
                    if (m == 0)
                    {
                        // normalisera efter den fullständiga vågen, samma förstärkning på alla nivåer
                        double peak = 1.0e-9;
                        for (int i = 0; i < kTableSize; ++i) peak = juce::jmax (peak, std::abs (small[(size_t) i].real()));
                        gain = (float) (1.0 / peak);
                    }
                    for (int i = 0; i < kTableSize; ++i) out[i] = (float) small[(size_t) i].real() * gain;
                    out[kTableSize] = out[0];
                }
            }
    }

    std::vector<float> data;
};

// Läser en tabell: blandar två grannvågor (frac) och interpolerar i vågen (t = 0..1).
inline float readFrames (const float* a, const float* b, float mix, float t) noexcept
{
    const float idx = t * (float) kTableSize;
    int i0 = (int) idx;
    if (i0 >= kTableSize) i0 = kTableSize - 1;
    const float fr = idx - (float) i0;
    const float va = a[i0] + fr * (a[i0 + 1] - a[i0]);
    const float vb = b[i0] + fr * (b[i0 + 1] - b[i0]);
    return va + mix * (vb - va);
}
} // namespace masugn
