// Sparar en bild av MASUGN:s panel som PNG (utvecklingshjälp).
#include "../src/PluginProcessor.h"
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    MasugnProcessor proc;
    proc.prepareToPlay (48000.0, 256);
    if (argc > 2) proc.loadPreset (juce::String (argv[2]).getIntValue());
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    ed->setBounds (0, 0, ed->getWidth(), ed->getHeight());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (argc > 1 ? argv[1] : "masugn.png");
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::cout << "sparade " << out.getFullPathName() << "\n";
    ed = nullptr;
    return 0;
}
