#include "MainComponent.h"


//==============================================================================
MainComponent::MainComponent()
: audioSetupComp(deviceManager, 0, 256, 0, 256, false, false, false, false)
{
    addAndMakeVisible(audioSetupComp);
    addAndMakeVisible(diagnosticsBox);

    diagnosticsBox.setMultiLine(true);
    diagnosticsBox.setReturnKeyStartsNewLine(true);
    diagnosticsBox.setReadOnly(true);
    diagnosticsBox.setScrollbarsShown(true);
    diagnosticsBox.setCaretVisible(false);
    diagnosticsBox.setPopupMenuEnabled(true);
    diagnosticsBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour (0x32ffffff));
    diagnosticsBox.setColour(juce::TextEditor::outlineColourId, juce::Colour (0x1c000000));
    diagnosticsBox.setColour(juce::TextEditor::shadowColourId, juce::Colour (0x16000000));

    cpuUsageLabel.setText("CPU Usage", juce::dontSendNotification);
    cpuUsageText.setJustificationType(juce::Justification::right);
    addAndMakeVisible (&cpuUsageText);
    addAndMakeVisible (&cpuUsageLabel);

    noiseLevelSlider.setRange(0.0, 0.25);
    noiseLevelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    noiseLevelLabel.setText("Noise Level", juce::dontSendNotification);

    addAndMakeVisible (noiseLevelSlider);
    addAndMakeVisible (noiseLevelLabel);

    inputGainSlider.setRange(0.0, 0.75);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    inputGainLabel.setText("Input Gain", juce::dontSendNotification);

    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(inputGainLabel);

    // Make sure you set the size of the component after
    // you add any child components.


    setSize (760, 450);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    deviceManager.addChangeListener(this);
    
    
    
    //
    // ---- Corelink Connection & Intialization ----
    //
    
    Corelink::DLLInit::Init();
    Corelink::DLLInit::setInitState(Corelink::Const::STREAM_STATE_UDP);
    Corelink::DLLInit::setOnUpdate(CorelinkInterface::corelinkServerUpdate);
    
//    corelinkInterface = new CorelinkInterface();
    
    //Connect To the Corelink Server
    if ( !Connect() ){ exit(-1); }
    
    try{
//        corelinkInterface = std::make_unique<CorelinkInterface>();
        
        Corelink::SendStream sender_stream = Corelink::Client::createSender("Holodeck", "audio", "",
                                                                            true, true, Corelink::Const::STREAM_STATE_SEND_UDP);
        corelinkInterface.attachStream(sender_stream);
        
        std::cout << "\n----Stream Data:\n" << Corelink::StreamData::getStreamData(STREAM_ID(sender_stream)) << std::endl;
        
        std::cout << "\n----Listing Active Streams:" << std::endl;
        std::vector<int> currStreamIDs = Corelink::Client::listStreams({ "Holodeck" }, { "audio" });
        for (int id : currStreamIDs) {
          std::cout << id << std::endl;
        }
        
        // Create our Receiver stream, and attach our callback function to it
        Corelink::RecvStream receiver_stream = Corelink::Client::createReceiver("Holodeck", {"audio", "receiver" },
                                                                                "", true, true,
                                                                                Corelink::Const::STREAM_STATE_RECV_UDP);
        
//        auto lambda  = [](void* extra, const int& receiver, const int& sender, const char* msg, const int& msgLen) {
//            if (extra != nullptr) {
//                auto ptr = static_cast<CorelinkInterface*>(extra);
//                ptr->corelinkRecvCallback(receiver, sender, msg, msgLen);
//            }
//        };
        
        std::cout << "Attaching callback function to receiver stream" << std::endl;
        receiver_stream.setOnReceive( receiveCallback );
         
        std::cout << "Subscribing to receiver stream" << std::endl;
        Corelink::Client::subscribe(STREAM_ID(receiver_stream), currStreamIDs[0]);
        
        std::cout << "\n----STARTING CORELINK SENDER----" << std::endl;
    }
    catch(Corelink::CorelinkException error) {
        std::cout << "CORELINK ERROR" << std::endl;
        std::cout << error.code << ":" << error.msg << std::endl;
      }
    
    startTimer(50);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    deviceManager.removeChangeListener(this);
    
    Corelink::Client::cleanup();
    
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    
    juce::String message;
    message << "Preparing to play audio...\n";
    message << " samplesPerBlockExpected = " << samplesPerBlockExpected << "\n";
    message << " sampleRate = " << sampleRate;
    juce::Logger::getCurrentLogger()->writeToLog (message);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
//    bufferToFill.clearActiveBufferRegion();
    
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    
    auto maxInputChannels =  activeInputChannels .getHighestBit() + 1;
    auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;
    
    //Note: OK to query UI objects, but neveer SET from inside thread.
    auto noiseLevel = (float) noiseLevelSlider.getValue();
    auto noiseLevelScale = noiseLevel * 2.0f;
    
    auto inputLevel = (float) inputGainSlider.getValue();
    auto inputLevelScale = inputLevel * 2.0f;
    
    auto bufToCorelink = new juce::AudioBuffer<float>(maxInputChannels, bufferToFill.numSamples);
    
    auto corelinkRecvBuf = new juce::AudioBuffer<float>(maxOutputChannels, bufferToFill.numSamples);
    
    
    
    for (auto channel = 0; channel < maxOutputChannels; ++channel)
    {
        // Check if maxNumInputChan is zero, if so zero output buffer.
        //   also checking for individual output channels + outputting silence if inactive
        if ((!activeInputChannels[channel]) || maxInputChannels == 0)
        {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
        }
        else
        {
            auto actualInputChannel = channel % maxInputChannels;
            
            if (! activeInputChannels[channel])
            {
                bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            }
            else
            {
                //get ptr to start sample in buffer for this channel at (channelNo, sampleIndex
                auto* iBuf = bufferToFill.buffer->getReadPointer(actualInputChannel, bufferToFill.startSample);
                auto* oBuf = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
                auto* oBufCL = bufToCorelink->getWritePointer(channel, bufferToFill.startSample);
                
                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
//                    oBuf[sample] = (iBuf[sample] * inputLevelScale - inputLevel)
//                        + (random.nextFloat() * noiseLevelScale - noiseLevel);
//                    oBuf[sample] = corelinkInterface->data.memoryBlock->
                    oBufCL[sample] = iBuf[sample];
                }
            }
        }
    }
    
    // ----Corelink Messaging----
    
    // Reinterpret buffer into a string, call send to send it over corelink
    
    const char* char_oBuf = reinterpret_cast<const char*>(&bufToCorelink);
    const std::string* stdString_oBuf = reinterpret_cast<std::string*>(&bufToCorelink);
    
//    size_t size = bufToCorelink->getNumSamples() * bufToCorelink->getNumChannels() * sizeof (float);
    
//    std::string string_oBuf(char_oBuf, size);
//    juce::CharPointer_UTF8 utf8_oBuf = string_oBuf.toUTF8();
//    const std::string* stdString_oBuf = reinterpret_cast<std::string*>(&string_oBuf);
    
    corelinkInterface.data.sender_stream->send(*stdString_oBuf);
    
    
    // Set the number of channels and samples within our data struct to reference by the receive callback
    corelinkInterface.data.numSamples = bufToCorelink->getNumSamples();
    corelinkInterface.data.numChannels = bufToCorelink->getNumChannels();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    
    
    Corelink::Client::cleanup();
    
    juce::Logger::getCurrentLogger()->writeToLog("Releasing Audio Resources");
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::grey);
    g.fillRect(getLocalBounds().removeFromRight(proportionOfWidth(0.4f)).removeFromBottom(getHeight()-70));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    
    noiseLevelSlider.setBounds(100, 10, getWidth() - 110, 20);
    noiseLevelLabel.setBounds(10, 10, 90, 20);
    
    inputGainSlider.setBounds(100, 40, getWidth()-110, 20);
    inputGainLabel.setBounds(10, 40, 90, 20);
    
    auto rect = getLocalBounds().removeFromBottom(getHeight()-70);
    
    audioSetupComp.setBounds( (rect.removeFromLeft(proportionOfWidth(0.6f))));
    rect.reduce(10, 10);
    
    auto topLine = (rect.removeFromTop(20));
    cpuUsageLabel.setBounds(topLine.removeFromLeft(topLine.getWidth()/2));
    cpuUsageText.setBounds(topLine);
    
    diagnosticsBox.setBounds(rect);
}

bool Connect(){
    
    Corelink::DLLInit::setServerCredentials("Testuser", "Testpassword");
    try {
        //For Local server:
        Corelink::Client::connect("127.0.0.1", 20012);
        //For NYU Server:
        // Corelink::Client::connect("Corelink.hpc.nyu.edu", 20012);
        return true;
    }
    catch (const Corelink::CorelinkException& err) {
        std::cout << "Error: " << err.what() << std::endl;
        std::cout << "Try again? (y/n)" << std::endl;
        
        std::string tmp1, tmp2 = "";
        while (true)
        {
            std::cin >> tmp1;
            if (tmp1 == "y" || tmp1 == "Y") { break; }
            else if (tmp1 == "n" || tmp1 == "N") { return false; }
        }
    }
    
    return true;
}
