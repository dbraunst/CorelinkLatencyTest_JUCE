#pragma once
#pragma once

#include <JuceHeader.h>
#include <Corelink.h>
#include "CorelinkInterface.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
public juce::ChangeListener,
private juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    static juce::String getListOfActiveBits (const juce::BigInteger& b)
    {
        juce::StringArray bits;
        
        for (auto i = 0; i <= b.getHighestBit(); ++i)
        {
            if (b[i])
            {
                bits.add (juce::String (i));
            }
        }
        
        return bits.joinIntoString(", ");
    }
    
    
    void logMessage (const juce::String& m)
    {
        diagnosticsBox.moveCaretToEnd();
        diagnosticsBox.insertTextAtCaret (m + juce::newLine);
    }
    
    void dumpDeviceInfo()
    {
        logMessage ("--------------------------------------");
        logMessage ("Current audio device type: " + (deviceManager.getCurrentDeviceTypeObject() != nullptr
                                                     ? deviceManager.getCurrentDeviceTypeObject()->getTypeName()
                                                     : "<none>"));

        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            logMessage ("Current audio device: "   + device->getName().quoted());
            logMessage ("Sample rate: "    + juce::String (device->getCurrentSampleRate()) + " Hz");
            logMessage ("Block size: "     + juce::String (device->getCurrentBufferSizeSamples()) + " samples");
            logMessage ("Bit depth: "      + juce::String (device->getCurrentBitDepth()));
            logMessage ("Input channel names: "    + device->getInputChannelNames().joinIntoString (", "));
            logMessage ("Active input channels: "  + getListOfActiveBits (device->getActiveInputChannels()));
            logMessage ("Output channel names: "   + device->getOutputChannelNames().joinIntoString (", "));
            logMessage ("Active output channels: " + getListOfActiveBits (device->getActiveOutputChannels()));
        }
        else
        {
            logMessage ("No audio device open");
        }
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        dumpDeviceInfo();
    }
    
private:
    //==============================================================================
    // Your private member variables go here...
    
    void timerCallback() override
    {
        auto cpu = deviceManager.getCpuUsage() * 100;
        cpuUsageText.setText (juce::String (cpu, 6) + " %", juce::dontSendNotification);
    }
    
    juce::Random random;
    
    juce::AudioDeviceSelectorComponent audioSetupComp;
    juce::Label cpuUsageLabel;
    juce::Label cpuUsageText;
    juce::TextEditor diagnosticsBox;
    
    juce::Slider noiseLevelSlider;
    juce::Label  noiseLevelLabel;
    
    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
