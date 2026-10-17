#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace masugn;

namespace
{
juce::String formatValue (const ParamDef& d, float v)
{
    const juce::String unit (d.unit);
    if (unit == "Hz")  return v < 1000.0f ? juce::String (v, v < 10.0f ? 2 : 0) + " Hz" : juce::String (v / 1000.0f, 1) + " kHz";
    if (unit == "s")   return v < 1.0f ? juce::String ((int) std::round (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s";
    if (unit == "dB")  return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
    if (unit == "c")   return (v > 0.0f ? "+" : "") + juce::String ((int) std::round (v)) + " c";
    if (unit == "st")  return d.kind == Kind::Int ? (v > 0.0f ? "+" : "") + juce::String ((int) std::round (v)) + " st" : juce::String (v, 1) + " st";
    if (d.kind == Kind::Int) return (v > 0.0f ? "+" : "") + juce::String ((int) std::round (v));
    if (d.min < 0.0f)  return (v > 0.0f ? "+" : "") + juce::String ((int) std::round (v * 100.0f)) + " %";
    return juce::String ((int) std::round ((v - d.min) / (d.max - d.min) * 100.0f)) + " %";
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        const auto name = juce::String::fromUTF8 (d.name);
        switch (d.kind)
        {
            case Kind::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > 0.0f) range.setSkewForCentre (d.centre);
                const ParamDef copy = d;
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, range, d.def,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([copy] (float v, int) { return formatValue (copy, v); })));
                break;
            }
            case Kind::Int:
            {
                const ParamDef copy = d;
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, name, (int) d.min, (int) d.max, (int) d.def,
                    juce::AudioParameterIntAttributes().withStringFromValueFunction ([copy] (int v, int) { return formatValue (copy, (float) v); })));
                break;
            }
            case Kind::Choice:
            {
                juce::StringArray items;
                for (auto* c : d.choices) items.add (juce::String::fromUTF8 (c));
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, items, (int) d.def));
                break;
            }
            case Kind::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, d.def > 0.5f));
                break;
        }
    }
    return layout;
}

// Vilka rattar varje tärning slår på
juce::Array<int> panelParams (int what)
{
    juce::Array<int> r;
    auto range = [&r] (int a, int b) { for (int k = a; k <= b; ++k) r.add (k); };
    switch (what)
    {
        case MasugnProcessor::DiceOsc:    range (O1Type, Binaural); break;
        case MasugnProcessor::DiceFilter: range (F1Type, Heat); break;
        case MasugnProcessor::DiceEnv:    range (FA, MR); break;
        case MasugnProcessor::DiceMod:    range (L1Wave, M4Amt); break;
        case MasugnProcessor::DiceFx:     range (Drive, Width); break;
        default:                          range (0, NumParams - 1); break;
    }
    return r;
}

} // namespace

//==============================================================================
MasugnProcessor::MasugnProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MASUGN", createLayout())
{
    for (int k = 0; k < NumParams; ++k)
        raw[(size_t) k] = apvts.getRawParameterValue (idOf (k));
    jassert (paramTableComplete());
    diceRng.setSeedRandomly();
    soundB = audioB = defaultValues();
    loadPreset (0);
    undoStack.clear();
}

bool MasugnProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MasugnProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate, juce::jmax (16, samplesPerBlock));
    setLatencySamples (engine.getLatency());
    keyboardState.reset();
}

std::array<float, NumParams> MasugnProcessor::readValues() const
{
    std::array<float, NumParams> v {};
    for (int k = 0; k < NumParams; ++k) v[(size_t) k] = raw[(size_t) k]->load();
    return v;
}

void MasugnProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;

    // tangentbordet på skärmen spelar också
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    if (panicPending.exchange (false)) engine.panic();

    // SMÄLT: blanda panelens ljud (A) med B
    const auto a = readValues();
    {
        const juce::SpinLock::ScopedTryLockType lock (bLock);
        if (lock.isLocked()) audioB = soundB;
    }
    auto p = toParams (meltValues (a, audioB, a[Smalt]), bpm);
    p.modWheel = modWheel;
    p.bend = bend;

    engine.process (buffer, midi, p, [this] (const juce::MidiMessage& m)
    {
        if (m.isPitchWheel()) bend = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
        else if (m.isControllerOfType (1)) modWheel = (float) m.getControllerValue() / 127.0f;
    });

    valveReduction.store (engine.reduction);
    level.store (buffer.getMagnitude (0, n));
}

//==============================================================================
void MasugnProcessor::setValues (const std::array<float, NumParams>& v, const juce::Array<int>& only)
{
    for (int k : only)
        if (auto* param = apvts.getParameter (idOf (k)))
        {
            const auto& d = paramDefs()[(size_t) k];
            const float val = juce::jlimit (d.min, d.max, v[(size_t) k]);
            param->setValueNotifyingHost (param->convertTo0to1 (val));
        }
}

void MasugnProcessor::loadPreset (int index)
{
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    pushUndo();
    juce::Array<int> all;
    for (int k = 0; k < NumParams; ++k) all.add (k);
    setValues (presetValues (index), all);
    setB (presetValuesB (index));
    currentPreset.store (index);
    apvts.state.setProperty ("group", list[(size_t) index].group, nullptr);
    panicPending.store (true);   // tystas i ljudtråden, inte här
}

juce::String MasugnProcessor::currentGroup() const
{
    const int i = currentPreset.load();
    const auto& list = factoryPresets();
    if (juce::isPositiveAndBelow (i, (int) list.size())) return list[(size_t) i].group;
    return apvts.state.getProperty ("group", "BAS").toString();
}

void MasugnProcessor::dice (int what)
{
    const auto& list = factoryPresets();
    const auto group = currentGroup();
    juce::Array<int> pool;
    for (int i = 0; i < (int) list.size(); ++i) if (list[(size_t) i].group == group) pool.add (i);
    if (pool.size() < 2) return;

    pushUndo();
    const int a = pool[diceRng.nextInt (pool.size())];
    int b = a;
    while (b == a) b = pool[diceRng.nextInt (pool.size())];
    const auto va = presetValues (a), vb = presetValues (b);
    auto now = readValues();
    auto out = now;
    const float t = 0.2f + 0.6f * diceRng.nextFloat();

    for (int k : panelParams (what))
    {
        const auto& d = paramDefs()[(size_t) k];
        if (k == Vol || k == Smalt) continue;
        if (d.kind != Kind::Float)
        {
            out[(size_t) k] = diceRng.nextBool() ? va[(size_t) k] : vb[(size_t) k];
            continue;
        }
        float x;
        if (isLogParam (k))
        {
            const float la = std::log (juce::jmax (1.0e-4f, va[(size_t) k])), lb = std::log (juce::jmax (1.0e-4f, vb[(size_t) k]));
            x = std::exp (la + (lb - la) * t + 0.25f * (diceRng.nextFloat() * 2.0f - 1.0f));
            if (k == Glide && va[(size_t) k] < 0.001f && vb[(size_t) k] < 0.001f) x = 0.0f;
        }
        else
        {
            x = va[(size_t) k] + (vb[(size_t) k] - va[(size_t) k]) * t;
            const bool wasZero = std::abs (va[(size_t) k]) < 1.0e-4f && std::abs (vb[(size_t) k]) < 1.0e-4f;
            if (! wasZero && k != O1Fine && k != O2Fine && k != O3Fine) x += (d.max - d.min) * 0.07f * (diceRng.nextFloat() * 2.0f - 1.0f);
        }
        out[(size_t) k] = juce::jlimit (d.min, d.max, x);
    }

    // säkerhetsgränser: det ska gå att lyssna på
    out[Heat]   = juce::jmin (out[Heat], 0.7f);
    out[Reso1]  = juce::jmin (out[Reso1], 0.85f);
    out[Reso2]  = juce::jmin (out[Reso2], 0.85f);
    out[Shimmer] = juce::jmin (out[Shimmer], 0.8f);
    out[RumMix] = juce::jmin (out[RumMix], 0.55f);
    out[Drive]  = juce::jmin (out[Drive], 0.6f);
    for (int m = 0; m < kModSlots; ++m)
        if ((int) std::round (out[modP (m, M1Dst)]) == DstPitch && group != juce::String::fromUTF8 ("DJUP"))
            out[modP (m, M1Amt)] = juce::jlimit (-0.05f, 0.05f, out[modP (m, M1Amt)]);   // ingen tjutande siren
    if (what == DiceAll)
    {
        out[Vol] = va[Vol] + (vb[Vol] - va[Vol]) * t;   // volymen följer med de två ljuden
        out[Mode] = va[Mode];                           // samma spelsätt som gruppen
    }

    juce::Array<int> all;
    for (int k = 0; k < NumParams; ++k) all.add (k);
    setValues (out, all);
    if (what == DiceAll)
    {
        // B blandas likadant, ur de två ljudens B
        const auto ba = presetValuesB (a), bb = presetValuesB (b);
        auto nb = out;
        for (int k = 0; k < NumParams; ++k)
        {
            const auto& d = paramDefs()[(size_t) k];
            if (k == Vol || k == Smalt) continue;
            if (d.kind != Kind::Float) nb[(size_t) k] = t < 0.5f ? ba[(size_t) k] : bb[(size_t) k];
            else if (isLogParam (k) && ba[(size_t) k] > 0.0f && bb[(size_t) k] > 0.0f)
                nb[(size_t) k] = std::exp (std::log (ba[(size_t) k]) + (std::log (bb[(size_t) k]) - std::log (ba[(size_t) k])) * t);
            else nb[(size_t) k] = ba[(size_t) k] + (bb[(size_t) k] - ba[(size_t) k]) * t;
        }
        setB (nb);
    }
    currentPreset.store (-1);
    apvts.state.setProperty ("group", group, nullptr);
    diceCount.fetch_add (1);
}

void MasugnProcessor::pushUndo()
{
    juce::MemoryBlock mb;
    getStateInformation (mb);
    undoStack.push_back (std::move (mb));
    while (undoStack.size() > 30) undoStack.pop_front();
}

bool MasugnProcessor::undo()
{
    if (undoStack.empty()) return false;
    auto mb = std::move (undoStack.back());
    undoStack.pop_back();
    setStateInformation (mb.getData(), (int) mb.getSize());
    diceCount.fetch_add (1);
    return true;
}

//==============================================================================
std::array<float, NumParams> MasugnProcessor::getB() const
{
    const juce::SpinLock::ScopedLockType lock (bLock);
    return soundB;
}

void MasugnProcessor::setB (const std::array<float, NumParams>& v)
{
    {
        const juce::SpinLock::ScopedLockType lock (bLock);
        soundB = v;
    }
    bChanged.fetch_add (1);
}

void MasugnProcessor::copyAtoB()
{
    pushUndo();
    setB (readValues());
}

void MasugnProcessor::swapAB()
{
    pushUndo();
    const auto a = readValues();
    auto b = getB();
    b[Vol] = a[Vol];        // volym och SMÄLT stannar på panelen
    b[Smalt] = a[Smalt];
    juce::Array<int> all;
    for (int k = 0; k < NumParams; ++k) all.add (k);
    setValues (b, all);
    setB (a);
    currentPreset.store (-1);
    diceCount.fetch_add (1);
}

void MasugnProcessor::diceB()
{
    // nytt B: A, men med tabellposition, filter, mutation, binaural och effekter knuffade åt något håll
    pushUndo();
    const auto a = readValues();
    auto b = a;
    auto nudge = [&] (int k, float amount)
    {
        const auto& d = paramDefs()[(size_t) k];
        const float span = d.max - d.min;
        b[(size_t) k] = juce::jlimit (d.min, d.max, a[(size_t) k] + span * amount * (diceRng.nextFloat() * 2.0f - 1.0f));
    };
    for (int o = 0; o < 3; ++o)
    {
        nudge (oscP (o, O1Pos), 0.6f);
        nudge (oscP (o, O1Form), 0.3f);
        nudge (oscP (o, O1MAmt), 0.5f);
    }
    for (int k : { Binaural, Reso1, Reso2, FEnv, Heat, Chorus, Shimmer, RumMix, Width }) nudge (k, 0.35f);
    for (int k : { Cut1, Cut2 })
        b[(size_t) k] = juce::jlimit (60.0f, 16000.0f, a[(size_t) k] * std::exp2 ((diceRng.nextFloat() * 2.0f - 1.0f) * 2.0f));
    b[Reso1] = juce::jmin (b[Reso1], 0.85f);
    b[Reso2] = juce::jmin (b[Reso2], 0.85f);
    b[Heat] = juce::jmin (b[Heat], 0.7f);
    b[RumMix] = juce::jmin (b[RumMix], 0.55f);
    setB (b);
}

void MasugnProcessor::storeB (juce::ValueTree& state) const
{
    juce::ValueTree t ("SOUND_B");
    const auto b = getB();
    for (int k = 0; k < NumParams; ++k) t.setProperty (juce::Identifier (idOf (k)), b[(size_t) k], nullptr);
    state.removeChild (state.getChildWithName ("SOUND_B"), nullptr);
    state.appendChild (t, nullptr);
}

void MasugnProcessor::restoreB (const juce::ValueTree& state)
{
    const auto t = state.getChildWithName ("SOUND_B");
    auto b = readValues();      // saknas något i B används panelens värde
    if (t.isValid())
        for (int k = 0; k < NumParams; ++k)
            if (t.hasProperty (juce::Identifier (idOf (k)))) b[(size_t) k] = (float) t.getProperty (juce::Identifier (idOf (k)));
    setB (b);
}

//==============================================================================
void MasugnProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    storeB (state);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void MasugnProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset.store ((int) state.getProperty ("preset", -1));
            apvts.replaceState (state);
            restoreB (state);
        }
}

juce::AudioProcessorEditor* MasugnProcessor::createEditor() { return new MasugnEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MasugnProcessor(); }
