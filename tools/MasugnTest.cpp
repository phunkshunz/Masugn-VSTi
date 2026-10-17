// MASUGN testprogram: spelar varje preset genom ljudmotorn, kontrollerar att inget går sönder
// och mäter volymen så att alla ljud kan volymjusteras lika.
//
//   MasugnTest                 -> rapport för alla presets
//   MasugnTest --trims         -> skriver nya volymer (dB) att klistra in i Presets.h
//   MasugnTest --wav <mapp>    -> sparar en wav per preset
#include <juce_audio_formats/juce_audio_formats.h>
#include "Presets.h"
#include <iostream>

using namespace masugn;

struct Result { float rmsDb = -100, peak = 0, maxReduction = 0, tailDb = -100; bool bad = false; };

static Result renderPreset (int index, juce::AudioBuffer<float>* keep, float extraDb = 0.0f)
{
    const double sr = 48000.0, bpm = 128.0;
    const int block = 256;
    auto values = presetValues (index);
    values[Vol] += extraDb;
    const auto p = toParams (values, bpm);

    Engine e;
    e.prepare (sr, block);

    // Fras: monoljud spelar en baslinje med accenter och bundna noter, polyljud spelar ackord.
    struct Ev { double beat; int note; float vel; bool on; };
    std::vector<Ev> evs;
    const bool poly = p.playMode == Poly;
    if (! poly)
    {
        const int line[16] = { 36, 36, 48, 36, 39, 36, 43, 41, 36, 48, 36, 46, 36, 39, 51, 36 };
        for (int i = 0; i < 16; ++i)
        {
            const double b = i * 0.25;
            const bool tie = (i % 4 == 2);
            const float vel = (i % 3 == 0) ? 0.9f : 0.6f;
            evs.push_back ({ b, line[i] + 12, vel, true });
            evs.push_back ({ b + (tie ? 0.30 : 0.18), line[i] + 12, 0.0f, false });
        }
    }
    else
    {
        const int chords[4][3] = { { 60, 63, 67 }, { 58, 62, 65 }, { 56, 60, 63 }, { 55, 58, 62 } };
        const bool longNotes = values[AA] > 0.1f || values[AS] > 0.3f;
        for (int c = 0; c < 4; ++c)
        {
            const double b = c * (longNotes ? 1.0 : 0.75);
            for (int n : chords[c])
            {
                evs.push_back ({ b, n, 0.8f, true });
                evs.push_back ({ b + (longNotes ? 0.9 : 0.2), n, 0.0f, false });
            }
        }
    }
    std::sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat || (a.beat == b.beat && ! a.on && b.on); });

    const int total = (int) (sr * 6.0);
    const double samplesPerBeat = sr * 60.0 / bpm;
    juce::AudioBuffer<float> out (2, total);
    juce::AudioBuffer<float> buf (2, block);
    Result r;
    double sumSq = 0.0; int count = 0;
    size_t next = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, n);
        juce::MidiBuffer midi;
        while (next < evs.size() && (int) (evs[next].beat * samplesPerBeat) < pos + n)
        {
            const auto& ev = evs[next++];
            const int at = juce::jmax (0, (int) (ev.beat * samplesPerBeat) - pos);
            midi.addEvent (ev.on ? juce::MidiMessage::noteOn (1, ev.note, ev.vel) : juce::MidiMessage::noteOff (1, ev.note), at);
        }
        e.process (view, midi, p, nullptr);
        r.maxReduction = juce::jmax (r.maxReduction, e.reduction);
        for (int ch = 0; ch < 2; ++ch)
        {
            out.copyFrom (ch, pos, view, ch, 0, n);
            for (int i = 0; i < n; ++i)
            {
                const float x = view.getSample (ch, i);
                if (! std::isfinite (x)) r.bad = true;
                r.peak = juce::jmax (r.peak, std::abs (x));
                if (pos + i > (int) (sr * 0.1) && pos + i < (int) (sr * 2.5)) { sumSq += x * x; ++count; }
            }
        }
    }
    juce::ignoreUnused (sumSq, count);
    // Ljudstyrka som en riktig mätare: 50 ms-fönster, bara där ljudet hörs (grind -50 dB, sedan -12 dB under medel).
    {
        const int win = (int) (sr * 0.05), from = (int) (sr * 0.1), to = (int) (sr * 3.0);
        std::vector<double> e;
        for (int s = from; s + win <= to; s += win)
        {
            double acc = 0.0;
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < win; ++i) { const double x = out.getSample (ch, s + i); acc += x * x; }
            e.push_back (acc / (2.0 * win));
        }
        auto meanAbove = [&] (double gate) { double a = 0.0; int c = 0; for (auto x : e) if (x > gate) { a += x; ++c; } return c > 0 ? a / c : 1e-10; };
        const double m1 = meanAbove (std::pow (10.0, -50.0 / 10.0));
        const double m2 = meanAbove (m1 * std::pow (10.0, -12.0 / 10.0));
        r.rmsDb = (float) (10.0 * std::log10 (m2 + 1e-12));
    }
    // svans: sista halvsekunden ska ha tystnat (inga hängande toner)
    double tail = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = total - (int) (sr * 0.5); i < total; ++i) tail += out.getSample (ch, i) * out.getSample (ch, i);
    r.tailDb = (float) juce::Decibels::gainToDecibels (std::sqrt (tail / (sr)), -100.0);
    if (keep != nullptr) *keep = out;
    return r;
}

int main (int argc, char** argv)
{
    const juce::String mode = argc > 1 ? argv[1] : "";
    const auto& list = factoryPresets();
    const float target = -16.0f;
    int problems = 0;

    for (int i = 0; i < (int) list.size(); ++i)
    {
        juce::AudioBuffer<float> audio;
        const auto r = renderPreset (i, mode == "--wav" ? &audio : nullptr);
        const float trim = juce::jlimit (-18.0f, 18.0f, target - (r.rmsDb - presetValues (i)[Vol] + (presetTrims().size() > (size_t) i ? presetTrims()[(size_t) i] : 0.0f) + 0.0f));
        const bool silent = r.rmsDb < -60.0f;
        const bool stuck = r.tailDb > -50.0f && list[(size_t) i].group != "YTA" && list[(size_t) i].group != "DJUP";
        if (r.bad || silent) ++problems;

        if (mode == "--trims")
        {
            // ny volym = nuvarande + skillnaden mot målet, men sänk tills SÄKERHETSVENTILEN knappt jobbar
            const float current = presetTrims().size() > (size_t) i ? presetTrims()[(size_t) i] : 0.0f;
            float extra = target - r.rmsDb;
            for (int guard = 0; guard < 30; ++guard)
            {
                const auto t = renderPreset (i, nullptr, extra);
                if (t.maxReduction <= 0.25f) break;
                extra -= 1.0f;
            }
            std::cout << juce::String (juce::jlimit (-18.0f, 18.0f, current + extra), 1) << "f, // " << list[(size_t) i].name << "\n";
            continue;
        }
        std::cout << juce::String (i).paddedLeft (' ', 3) << "  " << list[(size_t) i].group.paddedRight (' ', 9)
                  << list[(size_t) i].name.paddedRight (' ', 20)
                  << " rms " << juce::String (r.rmsDb, 1).paddedLeft (' ', 6)
                  << "  topp " << juce::String (r.peak, 2)
                  << "  ventil " << juce::String (r.maxReduction, 2)
                  << "  svans " << juce::String (r.tailDb, 0).paddedLeft (' ', 5)
                  << (r.bad ? "  OGILTIGT" : "") << (silent ? "  TYST" : "") << (stuck ? "  hänger?" : "") << "\n";
        juce::ignoreUnused (trim);

        if (mode == "--wav" && argc > 2)
        {
            juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
            dir.createDirectory();
            auto f = dir.getChildFile (juce::String (i).paddedLeft ('0', 2) + "_" + list[(size_t) i].name.replaceCharacter (' ', '_') + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            if (auto stream = std::unique_ptr<juce::FileOutputStream> (f.createOutputStream()))
                if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), 48000.0, 2, 24, {}, 0)))
                {
                    stream.release();
                    w->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
                }
        }
    }
    if (mode != "--trims") std::cout << "\n" << list.size() << " presets, " << problems << " problem\n";
    return problems == 0 ? 0 : 1;
}
