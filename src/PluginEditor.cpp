#include "PluginEditor.h"

using namespace masugn;

namespace
{
constexpr int kW = 1200, kH = 740;

juce::String typeName (int t)
{
    static const char* n[] = { "Vågtabell", "Partikel", "Resonator", "Kärna (FM)", "Analog" };
    return juce::String::fromUTF8 (n[juce::jlimit (0, 4, t)]);
}

// Vad POSITION- och FORM-ratten heter för varje oscillatortyp
std::pair<juce::String, juce::String> knobNames (int t)
{
    switch (t)
    {
        case TypeWave:      return { "POSITION", juce::String::fromUTF8 ("BÖJ") };
        case TypeParticle:  return { "POSITION", juce::String::fromUTF8 ("TÄTHET") };
        case TypeResonator: return { "MATERIAL", "KLANG" };
        case TypeKernel:    return { "INDEX", "RATIO" };
        default:            return { juce::String::fromUTF8 ("VÅG"), "SKEV" };
    }
}

juce::String typeTip (int t)
{
    switch (t)
    {
        case TypeWave:      return juce::String::fromUTF8 ("Vågtabell: POSITION glider genom tabellens 32 vågor. BÖJ trycker ihop vågen.");
        case TypeParticle:  return juce::String::fromUTF8 ("Partikel: små korn ur tabellen. POSITION väljer var, TÄTHET hur många och hur korta.");
        case TypeResonator: return juce::String::fromUTF8 ("Resonator: ett slag som ringer. MATERIAL går från sträng till stav till klocka. KLANG är hur länge det ringer.");
        case TypeKernel:    return juce::String::fromUTF8 ("Kärna: FM. INDEX är hur mycket modulatorn vrider. RATIO är modulatorns tonhöjd.");
        default:            return juce::String::fromUTF8 ("Analog: VÅG går från såg till fyrkant. SKEV gör vågen sned och nasal.");
    }
}
} // namespace

//==============================================================================
WaveDisplay::State WaveDisplay::state() const
{
    auto get = [this] (P k) { return proc.apvts.getRawParameterValue (MasugnProcessor::idOf (oscP (osc, k)))->load(); };
    State s;
    s.type = (int) std::round (get (O1Type));
    s.table = (int) std::round (get (O1Table));
    s.pos = get (O1Pos); s.form = get (O1Form); s.level = get (O1Level);
    s.mut = (int) std::round (get (O1Mut)); s.mamt = get (O1MAmt);
    return s;
}

void WaveDisplay::paint (juce::Graphics& g)
{
    using namespace MasugnColours;
    const auto s = state();
    auto r = getLocalBounds().toFloat();
    g.setColour (ground);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    const auto plot = r.reduced (10.0f, 8.0f).withTrimmedTop (10.0f);
    const auto dim = s.level < 0.001f;
    const auto hot = dim ? muted : sodium;

    // en våg som linje: f(t) ger -1..1
    auto wavePath = [] (juce::Rectangle<float> a, const std::function<float (float)>& f)
    {
        juce::Path p;
        const int nPts = juce::jmax (32, (int) a.getWidth());
        for (int i = 0; i <= nPts; ++i)
        {
            const float t = (float) i / (float) nPts;
            const float y = a.getCentreY() - juce::jlimit (-1.0f, 1.0f, f (t)) * a.getHeight() * 0.5f;
            if (i == 0) p.startNewSubPath (a.getX(), y); else p.lineTo (a.getX() + t * a.getWidth(), y);
        }
        return p;
    };

    if (s.type == TypeWave || s.type == TypeParticle)
    {
        // tabellen som en stapel vågor, den valda framme i orange
        const auto& bank = WaveBank::get();
        const int shown = 10;
        const float dx = 5.0f, dy = 1.5f;
        auto base = plot.withTrimmedRight (dx * (float) shown).withTrimmedTop (dy * (float) shown);
        const int cur = juce::jlimit (0, shown - 1, (int) std::round (s.pos * (float) (shown - 1)));
        for (int k = shown - 1; k >= 0; --k)
        {
            const int f = juce::jlimit (0, kFrames - 1, (int) std::round ((float) k / (float) (shown - 1) * (float) (kFrames - 1)));
            const float* data = bank.frame (s.table, 3, f);
            auto a = base.translated (dx * (float) k, -dy * (float) k);
            auto read = [data] (float t) { t -= std::floor (t); return data[juce::jlimit (0, kTableSize - 1, (int) (t * (float) kTableSize))]; };
            if (k != cur)
            {
                g.setColour (line2.withAlpha (0.9f));
                g.strokePath (wavePath (a, [read] (float t) { return read (t) * 0.9f; }), juce::PathStrokeType (1.0f));
                continue;
            }
            // den valda vågen ritas med MUTATORN, så man ser vad den gör
            const int mut = s.type == TypeWave && s.mamt > 0.001f ? s.mut : MutOff;
            const float amt = s.mamt;
            auto mutated = [=] (float t) -> float
            {
                const float tp = juce::MathConstants<float>::twoPi;
                switch (mut)
                {
                    case MutFm:    return read (t + amt * 0.32f * std::sin (tp * t));
                    case MutSync:  return read (t * (1.0f + amt * 6.0f));
                    case MutFold:  return fold (read (t), amt);
                    case MutPhase: return 0.7f * (read (t) - read (t + 0.5f - 0.45f * amt));
                    case MutSuper: return 0.5f * (read (t) + read (t * 1.0f + amt * 0.08f));
                    default:       return read (t);
                }
            };
            g.setColour (hot.withAlpha (0.25f));
            g.strokePath (wavePath (a, [mutated] (float t) { return mutated (t) * 0.9f; }), juce::PathStrokeType (4.5f));
            g.setColour (hot);
            g.strokePath (wavePath (a, [mutated] (float t) { return mutated (t) * 0.9f; }), juce::PathStrokeType (1.8f));
        }
        if (s.type == TypeParticle)
        {
            // prickar för kornen
            juce::Random rnd (s.table * 31 + (int) (s.pos * 100.0f));
            g.setColour (cold.withAlpha (dim ? 0.3f : 0.8f));
            const int count = 6 + (int) (s.form * 30.0f);
            for (int i = 0; i < count; ++i)
            {
                const float x = base.getX() + rnd.nextFloat() * base.getWidth();
                const float y = base.getY() + rnd.nextFloat() * base.getHeight();
                g.fillEllipse (x - 1.5f, y - 1.5f, 3.0f, 3.0f);
            }
        }
    }
    else if (s.type == TypeKernel)
    {
        const float idx = s.pos * s.pos * 7.0f, ratio = kernelRatio (s.form);
        const float tp = juce::MathConstants<float>::twoPi;
        g.setColour (hot);
        g.strokePath (wavePath (plot, [=] (float t) { return 0.9f * std::sin (tp * 2.0f * t + idx * std::sin (tp * 2.0f * ratio * t)); }),
                      juce::PathStrokeType (1.8f));
    }
    else if (s.type == TypeResonator)
    {
        // deltonerna som staplar, ljusare = ringer längre
        const float maxR = 34.0f;
        for (int k = 7; k >= 0; --k)
        {
            const float ratio = modeRatio (k, s.pos);
            const float x = plot.getX() + std::log2 (ratio) / std::log2 (maxR) * plot.getWidth();
            const float h = plot.getHeight() / std::pow ((float) (k + 1), 0.6f);
            g.setColour (hot.withAlpha (juce::jlimit (0.25f, 1.0f, 0.35f + s.form * 0.65f - (float) k * 0.05f)));
            g.fillRoundedRectangle (x - 2.0f, plot.getBottom() - h, 4.0f, h, 1.5f);
        }
    }
    else
    {
        const float morph = s.pos, w = 0.5f - 0.45f * s.form;
        g.setColour (hot);
        g.strokePath (wavePath (plot, [=] (float t0)
        {
            const float t = std::fmod (t0 * 2.0f, 1.0f);
            const float tw = s.form < 0.001f ? t : (t < w ? 0.5f * t / w : 0.5f + 0.5f * (t - w) / (1.0f - w));
            const float saw = 2.0f * tw - 1.0f, sq = t < (s.form < 0.001f ? 0.5f : w) ? 1.0f : -1.0f;
            return 0.85f * (saw + morph * (sq - saw));
        }), juce::PathStrokeType (1.8f));
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    juce::String label = "OSC " + juce::String (osc + 1) + juce::String::fromUTF8 ("  ·  ") + typeName (s.type);
    if (s.type == TypeWave || s.type == TypeParticle) label += juce::String::fromUTF8 ("  ·  ") + juce::String::fromUTF8 (tableNames()[juce::jlimit (0, kNumTables - 1, s.table)]);
    static const char* mutNames[] = { "", "FM", "Synk", "Veck", "Super", "Fasdiff" };
    if (s.mamt > 0.001f && s.mut > 0 && (s.type == TypeWave || (s.type == TypeAnalog && s.mut == MutFold)))
        label += juce::String::fromUTF8 ("  ·  ") + mutNames[juce::jlimit (0, 5, s.mut)];
    if (dim) label += juce::String::fromUTF8 ("  ·  av (NIVÅ 0)");
    g.drawText (label, getLocalBounds().reduced (8, 3).removeFromTop (12), juce::Justification::centredLeft, true);
}

//==============================================================================
MasugnEditor::MasugnEditor (MasugnProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p),
      diceOsc (juce::String::fromUTF8 ("Slumpa OSC")), diceFilter (juce::String::fromUTF8 ("Slumpa FILTER")),
      diceEnv (juce::String::fromUTF8 ("Slumpa KUVERT")), diceMod (juce::String::fromUTF8 ("Slumpa LFO och MOD")),
      diceFx (juce::String::fromUTF8 ("Slumpa EFFEKT")),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace MasugnColours;
    setLookAndFeel (&lnf);

    // överst: bläddra, tärning, ångra
    prevBtn.setButtonText (juce::String::fromUTF8 ("◀"));
    nextBtn.setButtonText (juce::String::fromUTF8 ("▶"));
    undoBtn.setButtonText (juce::String::fromUTF8 ("ÅNGRA"));
    prevBtn.onClick = [this] { step (-1); };
    nextBtn.onClick = [this] { step (1); };
    undoBtn.onClick = [this] { proc.undo(); };
    undoBtn.setTooltip (juce::String::fromUTF8 ("Backar senaste tärningsslaget eller ljudbytet"));
    for (auto* c : { (juce::Component*) &prevBtn, (juce::Component*) &nextBtn, (juce::Component*) &undoBtn, (juce::Component*) &presetBox })
        addAndMakeVisible (c);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.currentPreset.load()) proc.loadPreset (id - 1);
    };
    bigDice.setColour (sodium);
    bigDice.onClick = [this] { proc.dice (MasugnProcessor::DiceAll); };
    addAndMakeVisible (bigDice);

    // GJUTFORMARNA
    for (auto& g : presetGroups())
    {
        auto* b = groupBtns.add (new juce::TextButton (g));
        b->setClickingTogglesState (false);
        b->onClick = [this, g] { showGroup (g); };
        b->setTooltip (groupInfo (g));
        addAndMakeVisible (b);
    }
    list.setModel (this);
    list.setRowHeight (26);
    list.setColour (juce::ListBox::backgroundColourId, panel);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    // rattar (oscillatorernas egna rattar sköts av flikarna)
    const auto& defs = paramDefs();
    for (int k = 0; k < NumParams; ++k)
    {
        const auto kind = defs[(size_t) k].kind;
        if (kind != Kind::Float && kind != Kind::Int) continue;
        if (k >= O1Type && k <= O3Level) continue;
        auto kn = std::make_unique<Knob>();
        kn->attach (proc.apvts, MasugnProcessor::idOf (k), juce::String::fromUTF8 (defs[(size_t) k].name));
        addAndMakeVisible (*kn);
        knobs[(size_t) k] = std::move (kn);
    }
    knobs[(size_t) Cut1]->setLabel ("CUTOFF 1");
    knobs[(size_t) L1Rate]->setLabel ("FART");
    knobs[(size_t) L2Rate]->setLabel ("FART");
    for (int k : { FA, FD, FS, FR, Cut2, Reso2, FKey, Heat, Binaural }) knobs[(size_t) k]->setAccent (cold);
    for (int k : { MA, MD, MS, MR, Noise, Spread }) knobs[(size_t) k]->setAccent (concrete);
    for (int k : { L1Rate, L2Rate }) knobs[(size_t) k]->setAccent (cold);
    for (int k : { Drive, Chorus, Shimmer, RumMix, RumSize, Width }) knobs[(size_t) k]->setAccent (concrete);
    knobs[(size_t) Heat]->slider.setTooltip (juce::String::fromUTF8 ("HETTA: ljudet matas tillbaka in i filtret. Tjockt, varmt och smutsigt."));
    knobs[(size_t) Binaural]->slider.setTooltip (juce::String::fromUTF8 ("BINAURAL: varje ton får en egen vänster- och högerhalva som svävar isär. Gör ljudet stort och levande."));
    knobs[(size_t) Shimmer]->slider.setTooltip (juce::String::fromUTF8 ("SHIMMER: rumsklangen växer en oktav uppåt, som glöd."));
    knobs[(size_t) Width]->slider.setTooltip (juce::String::fromUTF8 ("BREDD: gör hela ljudet bredare i stereo"));
    // SMÄLT: den stora ratten
    knobs[(size_t) Smalt]->setLabel ({});
    knobs[(size_t) Smalt]->setAccent (cold);
    knobs[(size_t) Smalt]->slider.setTooltip (juce::String::fromUTF8 ("SMÄLT: glid från ljudet på panelen (A) till ljud B"));
    knobs[(size_t) Glide]->setAccent (concrete);
    knobs[(size_t) Vol]->setAccent (concrete);
    for (int m = 0; m < kModSlots; ++m)
    {
        auto& kn = *knobs[(size_t) modP (m, M1Amt)];
        kn.setLabel ({});
        kn.setAccent (cold);
        kn.slider.setTooltip (juce::String::fromUTF8 ("Hur mycket. Vänster om mitten = åt andra hållet."));
    }

    // OSC-flikar
    for (int o = 0; o < 3; ++o)
    {
        auto& t = oscTabs[(size_t) o];
        t.setButtonText ("OSC " + juce::String (o + 1));
        t.onClick = [this, o] { selectOsc (o); };
        addAndMakeVisible (t);
    }
    typeBox.addItemList ({ juce::String::fromUTF8 ("Vågtabell"), "Partikel", "Resonator", juce::String::fromUTF8 ("Kärna (FM)"), "Analog" }, 1);
    for (int t = 0; t < kNumTables; ++t) tableBox.addItem (juce::String::fromUTF8 (tableNames()[t]), t + 1);
    tableBox.setTooltip (juce::String::fromUTF8 ("Vilken vågtabell oscillatorn läser ur (Vågtabell och Partikel)"));
    mutBox.addItemList ({ juce::String::fromUTF8 ("Ingen mutator"), "FM", "Synk", "Veck", "Super", "Fasdiff" }, 1);
    mutBox.setTooltip (juce::String::fromUTF8 ("MUTATOR förvränger vågen. FM: skarp och metallisk. Synk: skrikig synk. Veck: vikt våg. "
                                               "Super: sju vågor isär stämda. Fasdiff: pulsbredd av vilken våg som helst. MUTATION-ratten är hur mycket."));
    addAndMakeVisible (typeBox);
    addAndMakeVisible (tableBox);
    addAndMakeVisible (mutBox);
    for (auto& k : oscKnobs) addAndMakeVisible (k);
    oscKnobs[KLevel].setAccent (sodium);
    addAndMakeVisible (display);

    addBox (f1Box, F1Type); addBox (f2Box, F2Type); addBox (routeBox, FRoute);
    addBox (modeBox, Mode);
    for (int l = 0; l < 2; ++l)
    {
        addBox (lfoWaveBox[(size_t) l], lfoP (l, L1Wave));
        addBox (lfoDivBox[(size_t) l], lfoP (l, L1Div));
        auto& b = syncBtn[(size_t) l];
        b.setButtonText ("TAKT");
        b.setClickingTogglesState (true);
        b.setTooltip (juce::String::fromUTF8 ("LFO:n följer DAW:ens tempo"));
        btnAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, MasugnProcessor::idOf (lfoP (l, L1Sync)), b));
        addAndMakeVisible (b);
    }
    for (int m = 0; m < kModSlots; ++m)
    {
        addBox (srcBox[(size_t) m], modP (m, M1Src));
        addBox (dstBox[(size_t) m], modP (m, M1Dst));
        srcBox[(size_t) m].setTooltip (juce::String::fromUTF8 ("KÄLLA: det som rör sig"));
        dstBox[(size_t) m].setTooltip (juce::String::fromUTF8 ("MÅL: det som ska röras"));
    }
    f1Box.setTooltip (juce::String::fromUTF8 ("Filter 1. Ladder 24 är fet och rund. Kam ger metalliska, stämda toner."));
    f2Box.setTooltip (juce::String::fromUTF8 ("Filter 2"));
    routeBox.setTooltip (juce::String::fromUTF8 ("Serie: filter 1 och sedan filter 2. Parallell: båda samtidigt, blandade."));
    modeBox.setTooltip (juce::String::fromUTF8 ("Poly: ackord. Mono: en ton i taget. Bunden: överlappande toner glider utan ny attack."));

    // SMÄLT-knappar
    copyBBtn.setButtonText (juce::String::fromUTF8 ("A → B"));
    swapBtn.setButtonText (juce::String::fromUTF8 ("BYT A/B"));
    diceBBtn.setButtonText (juce::String::fromUTF8 ("SLUMPA B"));
    copyBBtn.setTooltip (juce::String::fromUTF8 ("Kopiera ljudet på panelen till B. Ändra sedan panelen och smält mellan dem."));
    swapBtn.setTooltip (juce::String::fromUTF8 ("Byt plats: B hamnar på panelen så att du kan skruva på det"));
    diceBBtn.setTooltip (juce::String::fromUTF8 ("Gör ett nytt B nära ljudet på panelen"));
    copyBBtn.onClick = [this] { proc.copyAtoB(); };
    swapBtn.onClick = [this] { proc.swapAB(); };
    diceBBtn.onClick = [this] { proc.diceB(); };
    for (auto* b : { &copyBBtn, &swapBtn, &diceBBtn }) addAndMakeVisible (b);

    auto diceTo = [this] (DiceButton& d, int what, juce::Colour c) { d.setColour (c); d.onClick = [this, what] { proc.dice (what); }; addAndMakeVisible (d); };
    diceTo (diceOsc, MasugnProcessor::DiceOsc, sodium);
    diceTo (diceFilter, MasugnProcessor::DiceFilter, sodium);
    diceTo (diceEnv, MasugnProcessor::DiceEnv, cold);
    diceTo (diceMod, MasugnProcessor::DiceMod, cold);
    diceTo (diceFx, MasugnProcessor::DiceFx, concrete);

    // tangentbord
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9d6cf));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, ground);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line2);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, sodium);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, sodium.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, raised);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, sodium);
    keyboard.setAvailableRange (24, 108);
    keyboard.setOctaveForMiddleC (3);   // C3 = 60, som i Bitwig
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    addChildComponent (about);

    selectOsc (0);
    showGroup (proc.currentGroup());
    setSize (kW, kH);
    timerCallback();   // visa rätt ljud direkt, inte först efter första uppdateringen
    startTimerHz (30);
}

MasugnEditor::~MasugnEditor()
{
    setLookAndFeel (nullptr);
    list.setModel (nullptr);
}

void MasugnEditor::addBox (juce::ComboBox& b, int k)
{
    auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (MasugnProcessor::idOf (k)));
    if (param != nullptr) b.addItemList (param->choices, 1);
    boxAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, MasugnProcessor::idOf (k), b));
    addAndMakeVisible (b);
}

//==============================================================================
void MasugnEditor::selectOsc (int o)
{
    selOsc = o;
    for (int i = 0; i < 3; ++i) oscTabs[(size_t) i].setToggleState (i == o, juce::dontSendNotification);
    typeAtt.reset(); tableAtt.reset(); mutAtt.reset();
    typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, MasugnProcessor::idOf (oscP (o, O1Type)), typeBox);
    tableAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, MasugnProcessor::idOf (oscP (o, O1Table)), tableBox);
    mutAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, MasugnProcessor::idOf (oscP (o, O1Mut)), mutBox);
    const P ids[NumOscKnobs] = { O1Pos, O1Form, O1MAmt, O1Level, O1Oct, O1Semi, O1Fine };
    const char* names[NumOscKnobs] = { "POSITION", "FORM", "MUTATION", "NIVÅ", "OKTAV", "TON", "FIN" };
    for (int k = 0; k < NumOscKnobs; ++k)
        oscKnobs[(size_t) k].attach (proc.apvts, MasugnProcessor::idOf (oscP (o, ids[k])), juce::String::fromUTF8 (names[k]));
    shownType = -1;
    updateOscLabels();
    display.setOsc (o);
    repaint (oscR);
}

void MasugnEditor::updateOscLabels()
{
    const int t = (int) std::round (proc.apvts.getRawParameterValue (MasugnProcessor::idOf (oscP (selOsc, O1Type)))->load());
    if (t == shownType) return;
    shownType = t;
    const auto names = knobNames (t);
    oscKnobs[KPos].setLabel (names.first);
    oscKnobs[KForm].setLabel (names.second);
    tableBox.setEnabled (t == TypeWave || t == TypeParticle);
    mutBox.setEnabled (t == TypeWave || t == TypeAnalog);
    oscKnobs[KMut].setEnabled (t == TypeWave || t == TypeAnalog);
    typeBox.setTooltip (typeTip (t));
}

void MasugnEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& l = factoryPresets();
    juce::String group;
    for (int i = 0; i < (int) l.size(); ++i)
    {
        if (l[(size_t) i].group != group)
        {
            group = l[(size_t) i].group;
            presetBox.addSectionHeading (group + juce::String::fromUTF8 ("  ·  ") + groupInfo (group));
        }
        presetBox.addItem (l[(size_t) i].name, i + 1);
    }
}

void MasugnEditor::step (int delta)
{
    const int count = (int) factoryPresets().size();
    int i = proc.currentPreset.load();
    if (i < 0)
    {
        // efter tärning: börja från gruppens första ljud
        i = 0;
        for (int k = 0; k < count; ++k) if (factoryPresets()[(size_t) k].group == proc.currentGroup()) { i = k; break; }
        proc.loadPreset (i);
        return;
    }
    proc.loadPreset ((i + delta + count) % count);
}

void MasugnEditor::showGroup (const juce::String& g)
{
    shownGroup = g;
    rows.clear();
    const auto& l = factoryPresets();
    for (int i = 0; i < (int) l.size(); ++i) if (l[(size_t) i].group == g) rows.add (i);
    for (auto* b : groupBtns) b->setToggleState (b->getButtonText() == g, juce::dontSendNotification);
    list.updateContent();
    list.repaint();
    repaint (vaultR);
}

int MasugnEditor::getNumRows() { return rows.size(); }

void MasugnEditor::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool)
{
    using namespace MasugnColours;
    if (! juce::isPositiveAndBelow (row, rows.size())) return;
    const int index = rows[row];
    const bool current = index == proc.currentPreset.load();
    auto r = juce::Rectangle<int> (0, 0, width, height).reduced (2, 1);
    if (current)
    {
        g.setColour (sodium.withAlpha (0.9f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }
    else if (row % 2 == 0)
    {
        g.setColour (raised.withAlpha (0.5f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }
    g.setColour (current ? ground : muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (juce::String (row + 1).paddedLeft ('0', 2), r.removeFromLeft (30), juce::Justification::centred);
    g.setColour (current ? ground : text);
    g.setFont (juce::FontOptions (13.0f, current ? juce::Font::bold : juce::Font::plain));
    g.drawText (factoryPresets()[(size_t) index].name, r, juce::Justification::centredLeft, true);
}

void MasugnEditor::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, rows.size())) proc.loadPreset (rows[row]);
}

//==============================================================================
void MasugnEditor::paint (juce::Graphics& g)
{
    using namespace MasugnColours;
    g.fillAll (ground);

    // glöd underifrån, som från en ugnslucka
    {
        juce::ColourGradient heatGlow (cold.withAlpha (0.10f), (float) kW * 0.5f, (float) kH, ground.withAlpha (0.0f), (float) kW * 0.5f, (float) kH * 0.45f, true);
        g.setGradientFill (heatGlow);
        g.fillRect (getLocalBounds());
    }

    // logga: glödande, gul överst och röd nedtill
    {
        auto r = logoArea.withTrimmedBottom (14).toFloat();
        g.setGradientFill (juce::ColourGradient (sodium.brighter (0.3f), r.getX(), r.getY(), cold, r.getX(), r.getBottom(), false));
        g.setFont (juce::FontOptions (44.0f, juce::Font::bold).withHorizontalScale (0.85f));
        g.drawText ("MASUGN", logoArea.withTrimmedBottom (14), juce::Justification::centredLeft);
    }
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (juce::String::fromUTF8 ("MASKINHALLEN · PHUNKSHUNZ AUDIO"), logoArea.withTrimmedTop (logoArea.getHeight() - 14), juce::Justification::centredLeft);

    drawPanel (g, vaultR, "GJUTFORMAR", juce::String ((int) factoryPresets().size()) + " ljud");
    drawPanel (g, oscR, "OSC");
    drawPanel (g, smaltR, juce::String::fromUTF8 ("SMÄLT"), juce::String::fromUTF8 ("ljud A → ljud B"));
    drawPanel (g, filtR, "FILTER");
    drawPanel (g, fxR, "EFFEKT");
    drawPanel (g, envR, "KUVERT", juce::String::fromUTF8 ("röd filter · gul volym · grå 3"));
    drawPanel (g, lfoR, "LFO");
    drawPanel (g, modR, "MOD", juce::String::fromUTF8 ("källa → mål"));

    // SMÄLT: glödande ram och A / B vid ratten
    {
        // glöd bakom ratten, starkare ju mer man smälter
        const float m = proc.apvts.getRawParameterValue (MasugnProcessor::idOf (Smalt))->load();
        const auto kc = knobs[(size_t) Smalt]->getBounds().toFloat().withTrimmedBottom (13.0f).getCentre();
        juce::ColourGradient glow (cold.withAlpha (0.10f + 0.40f * m), kc.x, kc.y, cold.withAlpha (0.0f), kc.x + 95.0f, kc.y, true);
        glow.addColour (0.35, sodium.withAlpha (0.06f + 0.22f * m));
        g.setGradientFill (glow);
        g.fillEllipse (juce::Rectangle<float> (176.0f, 176.0f).withCentre (kc));

        g.setColour (cold.withAlpha (0.55f));
        g.drawRoundedRectangle (smaltR.toFloat().reduced (0.5f), 6.0f, 1.2f);
        const auto kb = knobs[(size_t) Smalt]->getBounds();
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.setColour (sodium);
        g.drawText ("A", kb.getX() - 4, kb.getBottom() - 34, 20, 20, juce::Justification::centred);
        g.setColour (cold);
        g.drawText ("B", kb.getRight() - 16, kb.getBottom() - 34, 20, 20, juce::Justification::centred);
        g.setColour (muted);
        g.setFont (juce::FontOptions (10.0f));
        g.drawFittedText (juce::String::fromUTF8 ("Vrid för att smälta ljudet på panelen mot B. Tryck A → B och skruva vidare."),
                          smaltR.getX() + 12, smaltR.getBottom() - 40, smaltR.getWidth() - 24, 32, juce::Justification::centredLeft, 3);
    }

    // gruppens beskrivning
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (groupInfo (shownGroup), vaultR.getX() + 12, vaultR.getY() + 98, vaultR.getWidth() - 24, 16, juce::Justification::centredLeft);

    // små rubriker
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.setColour (muted);
    for (int l = 0; l < 2; ++l)
        g.drawText ("LFO " + juce::String (l + 1), lfoR.getX() + 12, lfoR.getY() + 30 + l * 108, 60, 10, juce::Justification::centredLeft);
    for (int m = 0; m < kModSlots; ++m)
        g.drawText (juce::String (m + 1), modR.getX() + 4, modR.getY() + 40 + m * 53, 8, 20, juce::Justification::centred);

    // NIVÅ: blir röd när SÄKERHETSVENTILEN tar i
    {
        auto r = meterR;
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (juce::String::fromUTF8 (shownValve > 0.02f ? "NIVÅ · VENTIL" : "NIVÅ"), r.removeFromTop (11), juce::Justification::centredLeft);
        auto b = r.removeFromTop (7).toFloat();
        g.setColour (line);
        g.fillRoundedRectangle (b, 3.0f);
        const float levelDb = juce::Decibels::gainToDecibels (shownLevel, -60.0f);
        g.setColour (shownValve > 0.02f ? cold : sodium);
        g.fillRoundedRectangle (b.withWidth (b.getWidth() * juce::jlimit (0.0f, 1.0f, (levelDb + 60.0f) / 60.0f)), 3.0f);
    }
}

void MasugnEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<int> params, int cols, int cellH)
{
    const int cellW = area.getWidth() / cols;
    int i = 0;
    for (int k : params)
    {
        const int c = i % cols, r = i / cols;
        if (k >= 0 && knobs[(size_t) k] != nullptr)
            knobs[(size_t) k]->setBounds (area.getX() + c * cellW, area.getY() + r * cellH, cellW, cellH);
        ++i;
    }
}

void MasugnEditor::resized()
{
    about.setBounds (getLocalBounds());

    topR = { 16, 8, kW - 32, 56 };
    logoArea = { 16, 10, 260, 52 };
    prevBtn.setBounds (300, 20, 32, 32);
    presetBox.setBounds (338, 20, 360, 32);
    nextBtn.setBounds (704, 20, 32, 32);
    bigDice.setBounds (748, 14, 44, 44);
    undoBtn.setBounds (804, 20, 80, 32);

    // uppe till höger: spelsätt, nivå, glid och volym
    modeBox.setBounds (904, 12, 104, 24);
    meterR = { 904, 42, 104, 20 };
    knobs[(size_t) Glide]->setBounds (1020, 4, 78, 62);
    knobs[(size_t) Vol]->setBounds (1100, 4, 78, 62);

    vaultR = { 16, 72, 260, 562 };
    {
        auto g = vaultR.reduced (12).withTrimmedTop (24);
        for (int i = 0; i < groupBtns.size(); ++i)
            groupBtns[i]->setBounds (g.getX() + (i % 3) * (g.getWidth() / 3) + 2, g.getY() + (i / 3) * 30, g.getWidth() / 3 - 4, 26);
        list.setBounds (vaultR.getX() + 8, vaultR.getY() + 120, vaultR.getWidth() - 16, vaultR.getHeight() - 128);
    }

    const int x0 = 288, right = kW - 16;
    oscR   = { x0, 72, 420, 300 };
    smaltR = { oscR.getRight() + 12, 72, 200, 300 };
    filtR  = { smaltR.getRight() + 12, 72, right - (smaltR.getRight() + 12), 300 };
    envR   = { x0, 384, 290, 250 };
    lfoR   = { envR.getRight() + 12, 384, 170, 250 };
    modR   = { lfoR.getRight() + 12, 384, 236, 250 };
    fxR    = { modR.getRight() + 12, 384, right - (modR.getRight() + 12), 250 };

    auto diceAt = [] (DiceButton& d, juce::Rectangle<int> panel, int x) { d.setBounds (panel.getX() + x, panel.getY() + 7, 20, 20); };
    diceAt (diceOsc, oscR, 46); diceAt (diceFilter, filtR, 64); diceAt (diceFx, fxR, 70); diceAt (diceEnv, envR, 80);
    diceAt (diceMod, modR, 48);

    // OSC
    for (int o = 0; o < 3; ++o)
        oscTabs[(size_t) o].setBounds (oscR.getRight() - 12 - (3 - o) * 62 + 4, oscR.getY() + 6, 58, 22);
    {
        const int third = (oscR.getWidth() - 24) / 3;
        typeBox.setBounds (oscR.getX() + 12, oscR.getY() + 34, third - 4, 26);
        tableBox.setBounds (oscR.getX() + 12 + third + 2, oscR.getY() + 34, third - 4, 26);
        mutBox.setBounds (oscR.getX() + 12 + 2 * third + 4, oscR.getY() + 34, third - 4, 26);
        display.setBounds (oscR.getX() + 12, oscR.getY() + 66, oscR.getWidth() - 24, 66);
        auto area = juce::Rectangle<int> (oscR.getX() + 12, oscR.getY() + 136, oscR.getWidth() - 24, 160);
        const int cw = area.getWidth() / 5, ch = 80;
        auto cell = [&] (int slot) { return juce::Rectangle<int> (area.getX() + (slot % 5) * cw, area.getY() + (slot / 5) * ch, cw, ch); };
        oscKnobs[KPos].setBounds (cell (0));
        oscKnobs[KForm].setBounds (cell (1));
        oscKnobs[KMut].setBounds (cell (2));
        oscKnobs[KLevel].setBounds (cell (3));
        knobs[(size_t) Noise]->setBounds (cell (4));
        oscKnobs[KOct].setBounds (cell (5));
        oscKnobs[KSemi].setBounds (cell (6));
        oscKnobs[KFine].setBounds (cell (7));
        knobs[(size_t) Spread]->setBounds (cell (8));
        knobs[(size_t) Binaural]->setBounds (cell (9));
    }

    // SMÄLT
    knobs[(size_t) Smalt]->setBounds (smaltR.getX() + 22, smaltR.getY() + 30, smaltR.getWidth() - 44, 150);
    copyBBtn.setBounds (smaltR.getX() + 12, smaltR.getY() + 188, (smaltR.getWidth() - 30) / 2, 26);
    swapBtn.setBounds (smaltR.getX() + 18 + (smaltR.getWidth() - 30) / 2, smaltR.getY() + 188, (smaltR.getWidth() - 30) / 2, 26);
    diceBBtn.setBounds (smaltR.getX() + 12, smaltR.getY() + 220, smaltR.getWidth() - 24, 26);

    // FILTER
    {
        const int half = (filtR.getWidth() - 24) / 2;
        routeBox.setBounds (filtR.getRight() - 12 - 100, filtR.getY() + 6, 100, 22);
        f1Box.setBounds (filtR.getX() + 12, filtR.getY() + 34, half - 3, 26);
        f2Box.setBounds (filtR.getX() + 12 + half + 3, filtR.getY() + 34, half - 3, 26);
        layoutKnobs (filtR.reduced (12, 0).withTrimmedTop (66), { Cut1, Reso1, FEnv, Cut2, Reso2, FKey, FDrive, Heat }, 3, 76);
    }

    // EFFEKT
    layoutKnobs (fxR.reduced (10, 0).withTrimmedTop (32), { Drive, Chorus, Shimmer, RumMix, RumSize, Width }, 2, 72);

    // KUVERT
    layoutKnobs (envR.reduced (12, 0).withTrimmedTop (32), { FA, FD, FS, FR, AA, AD, AS, AR, MA, MD, MS, MR }, 4, 72);

    // LFO
    for (int l = 0; l < 2; ++l)
    {
        const int by = lfoR.getY() + 44 + l * 108;
        lfoWaveBox[(size_t) l].setBounds (lfoR.getX() + 12, by, 92, 24);
        syncBtn[(size_t) l].setBounds (lfoR.getRight() - 12 - 50, by, 50, 24);
        auto cell = juce::Rectangle<int> (lfoR.getX() + 12, by + 28, lfoR.getWidth() - 24, 70);
        knobs[(size_t) lfoP (l, L1Rate)]->setBounds (cell);
        lfoDivBox[(size_t) l].setBounds (cell.withSizeKeepingCentre (110, 26).withY (cell.getY() + 18));
    }

    // MOD
    for (int m = 0; m < kModSlots; ++m)
    {
        const int ry = modR.getY() + 34 + m * 53;
        srcBox[(size_t) m].setBounds (modR.getX() + 14, ry + 8, 80, 24);
        dstBox[(size_t) m].setBounds (modR.getX() + 98, ry + 8, 92, 24);
        knobs[(size_t) modP (m, M1Amt)]->setBounds (modR.getX() + 192, ry, 40, 50);
    }

    keyboard.setBounds (16, 646, kW - 32, kH - 654);
    keyboard.setKeyWidth ((float) (kW - 32 - 30) / 36.0f);  // fem oktaver syns
    keyboard.setLowestVisibleKey (36);
}

void MasugnEditor::mouseDown (const juce::MouseEvent& e)
{
    if (logoArea.contains (e.getPosition()))
    {
        about.setVisible (true);
        about.toFront (false);
    }
}

void MasugnEditor::timerCallback()
{
    const int cur = proc.currentPreset.load();
    const int dc = proc.diceCount.load();
    if (cur != lastPreset || dc != lastDice)
    {
        lastPreset = cur; lastDice = dc;
        if (cur >= 0) presetBox.setSelectedId (cur + 1, juce::dontSendNotification);
        else presetBox.setText (juce::String::fromUTF8 ("Slumpat ljud · ") + proc.currentGroup(), juce::dontSendNotification);
        if (proc.currentGroup() != shownGroup) showGroup (proc.currentGroup());
        const int row = rows.indexOf (cur);
        if (row >= 0) list.scrollToEnsureRowIsOnscreen (row);
        list.repaint();
        repaint (topR);
    }

    for (int l = 0; l < 2; ++l)
    {
        const bool sync = proc.apvts.getRawParameterValue (MasugnProcessor::idOf (lfoP (l, L1Sync)))->load() > 0.5f;
        knobs[(size_t) lfoP (l, L1Rate)]->setVisible (! sync);
        lfoDivBox[(size_t) l].setVisible (sync);
    }
    // MÄNGD-ratten är bara tänd när både källa och mål är valda
    for (int m = 0; m < kModSlots; ++m)
    {
        const bool on = proc.apvts.getRawParameterValue (MasugnProcessor::idOf (modP (m, M1Src)))->load() > 0.5f
                     && proc.apvts.getRawParameterValue (MasugnProcessor::idOf (modP (m, M1Dst)))->load() > 0.5f;
        knobs[(size_t) modP (m, M1Amt)]->setAlpha (on ? 1.0f : 0.4f);
    }
    // flikarna: en prick lyser för oscillatorer som hörs
    for (int o = 0; o < 3; ++o)
    {
        const bool audible = proc.apvts.getRawParameterValue (MasugnProcessor::idOf (oscP (o, O1Level)))->load() > 0.001f;
        oscTabs[(size_t) o].setButtonText ((audible ? juce::String::fromUTF8 ("● ") : juce::String()) + "OSC " + juce::String (o + 1));
    }
    updateOscLabels();
    display.refresh();
    {
        const float m = proc.apvts.getRawParameterValue (MasugnProcessor::idOf (Smalt))->load();
        if (proc.bChanged.load() != lastB || std::abs (m - lastSmalt) > 0.004f)
        {
            lastB = proc.bChanged.load(); lastSmalt = m;
            repaint (smaltR);
        }
    }
    undoBtn.setEnabled (proc.canUndo());

    const float lv = proc.level.load(), vr = proc.valveReduction.load();
    shownLevel = lv > shownLevel ? lv : shownLevel * 0.85f;
    shownValve = vr > shownValve ? vr : shownValve * 0.85f;
    repaint (meterR);
}
