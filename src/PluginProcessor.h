#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Presets.h"
#include <deque>

// MASUGN: vågtabells-synth med partiklar, resonatorer och FM. Phunkshunz AUDIO, serien MASKINHALLEN.
class MasugnProcessor : public juce::AudioProcessor
{
public:
    MasugnProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MASUGN"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    // Ljuden syns också i DAW:ens preset-lista
    int getNumPrograms() override { return (int) masugn::factoryPresets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, currentPreset.load()); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override
    {
        const auto& list = masugn::factoryPresets();
        return juce::isPositiveAndBelow (index, (int) list.size()) ? list[(size_t) index].group + " - " + list[(size_t) index].name : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    void loadPreset (int index);
    std::atomic<int> currentPreset { -1 };

    // TÄRNINGAR: blandar ljud ur samma grupp och skruvar lite, så att det blir något nytt men aldrig skrik.
    enum Dice { DiceAll, DiceOsc, DiceFilter, DiceEnv, DiceMod, DiceFx };
    void dice (int what);
    bool undo();
    bool canUndo() const { return ! undoStack.empty(); }
    std::atomic<int> diceCount { 0 };

    juce::String currentGroup() const;

    // SMÄLT: ljud B. Ratten SMÄLT glider från panelens ljud (A) till B.
    void copyAtoB();          // B blir som A
    void swapAB();            // byt plats på A och B
    void diceB();             // slumpa ett nytt B nära A
    std::array<float, masugn::NumParams> getB() const;
    std::atomic<int> bChanged { 0 };

    std::atomic<float> level { 0.0f }, valveReduction { 0.0f };

    static juce::String idOf (int p) { return masugn::paramDefs()[(size_t) p].id; }

private:
    std::array<float, masugn::NumParams> readValues() const;
    void setValues (const std::array<float, masugn::NumParams>& v, const juce::Array<int>& only);
    void pushUndo();
    void setB (const std::array<float, masugn::NumParams>& v);
    void storeB (juce::ValueTree& state) const;
    void restoreB (const juce::ValueTree& state);

    masugn::Engine engine;
    std::array<std::atomic<float>*, masugn::NumParams> raw {};
    float modWheel = 0.0f, bend = 0.0f;
    std::atomic<bool> panicPending { false };
    double sr = 44100.0;
    std::deque<juce::MemoryBlock> undoStack;
    juce::Random diceRng;

    // ljud B: skrivs från panelen, läses i ljudtråden (låset hålls bara en kort kopiering)
    std::array<float, masugn::NumParams> soundB {};
    mutable juce::SpinLock bLock;
    std::array<float, masugn::NumParams> audioB {};   // ljudtrådens kopia

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasugnProcessor)
};
