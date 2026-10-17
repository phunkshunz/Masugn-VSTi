#pragma once

#include "PluginProcessor.h"
#include "Gui.h"
#include "About.h"

// Skärmen i OSC-panelen: visar vågtabellen (eller FM-vågen, resonatorns deltoner, den analoga vågen)
// för den oscillator som är vald.
class WaveDisplay : public juce::Component
{
public:
    explicit WaveDisplay (MasugnProcessor& p) : proc (p) {}

    void setOsc (int o) { osc = o; repaint(); }

    // ritas bara om när något har ändrats
    void refresh()
    {
        const auto s = state();
        if (s != last) { last = s; repaint(); }
    }

    void paint (juce::Graphics& g) override;

private:
    struct State
    {
        int type = -1, table = -1; float pos = -1.0f, form = -1.0f, level = -1.0f;
        bool operator!= (const State& o) const { return type != o.type || table != o.table || pos != o.pos || form != o.form || level != o.level; }
    };
    State state() const;

    MasugnProcessor& proc;
    int osc = 0;
    State last;
};

// MASUGN: panelen. Till vänster GJUTFORMARNA där ljuden förvaras, till höger rattarna, längst ned ett tangentbord.
class MasugnEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::ListBoxModel
{
public:
    explicit MasugnEditor (MasugnProcessor&);
    ~MasugnEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // GJUTFORMARNA: listan med ljud i vald grupp
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    void showGroup (const juce::String& g);
    void refreshPresetBox();
    void step (int delta);
    void selectOsc (int o);
    void updateOscLabels();
    void layoutKnobs (juce::Rectangle<int> area, std::initializer_list<int> params, int cols, int cellH);
    void addBox (juce::ComboBox& b, int param);

    MasugnProcessor& proc;
    MasugnLnF lnf;

    // överst
    juce::Rectangle<int> logoArea;
    juce::TextButton prevBtn, nextBtn, undoBtn;
    juce::ComboBox presetBox;
    DiceButton bigDice { juce::String::fromUTF8 ("SLUMPA: blandar två ljud ur gruppen till ett nytt") };

    // GJUTFORMARNA
    juce::OwnedArray<juce::TextButton> groupBtns;
    juce::ListBox list;
    juce::String shownGroup;
    juce::Array<int> rows;

    // OSC: tre flikar som delar samma rattar
    std::array<juce::TextButton, 3> oscTabs;
    int selOsc = 0, shownType = -1;
    juce::ComboBox typeBox, tableBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt, tableAtt;
    enum { KPos, KForm, KLevel, KOct, KSemi, KFine, NumOscKnobs };
    std::array<Knob, NumOscKnobs> oscKnobs;
    WaveDisplay display;

    // övriga rattar och väljare
    std::array<std::unique_ptr<Knob>, masugn::NumParams> knobs;
    juce::ComboBox f1Box, f2Box, routeBox, modeBox, ekoDivBox;
    std::array<juce::ComboBox, 2> lfoWaveBox, lfoDivBox;
    std::array<juce::TextButton, 2> syncBtn;
    std::array<juce::ComboBox, masugn::kModSlots> srcBox, dstBox;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtt;

    DiceButton diceOsc, diceFilter, diceEnv, diceMod, diceFx;

    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 700 };   // visar förklaringen när musen vilar på något
    AboutOverlay about { "MASUGN", juce::String::fromUTF8 ("Vågtabeller, partiklar, resonatorer och FM") };

    juce::Rectangle<int> topR, vaultR, oscR, filtR, fxR, envR, lfoR, modR, spelR, meterR;
    int lastPreset = -2, lastDice = -1;
    float shownLevel = 0.0f, shownValve = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasugnEditor)
};
