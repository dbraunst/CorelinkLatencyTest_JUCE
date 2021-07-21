/*
  ==============================================================================

    CorelinkInterface.cpp
    Created: 13 Jul 2021 10:02:10pm
    Author:  Daniel Braunstein

  ==============================================================================
*/

#include "CorelinkInterface.h"

std::mutex printLock;


// Connect to Corelink with predetermined credentials + IP address,
//   throw error if failed
bool CorelinkInterface::Connect(){
    
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

// Connect to Corelink while prompting user for User/Pass/IP
bool CorelinkInterface::InitLoop()
{
    //TODO: Implement this
    return false;
}

void CorelinkInterface::openCorelink()
{
    std::cout << "Opening Corelink..." << std::endl;
    Corelink::DLLInit::Init();
    Corelink::DLLInit::setInitState(Corelink::Const::STREAM_STATE_UDP);
    Corelink::DLLInit::setOnUpdate(corelinkServerUpdate);
    
    std::cout << "Corelink opened successfully" << std::endl;
}

void CorelinkInterface::closeCorelink()
{
    std::cout << "Cleaning up corelink..." << std::endl;
    Corelink::Client::cleanup();
    std::cout << "Corelink cleaned up successfully" << std::endl;
}

// Callback function for when a sender stream appears on the server that this client is able
//  to listen to. Here we automatically subscribe for testing purposes, but that can be
//  an option presented to users
void CorelinkInterface::corelinkServerUpdate(const int& receiverID, const int& senderID){
    
    Corelink::Client::subscribe(receiverID, senderID);
    std::lock_guard<std::mutex> lck(printLock);
    std::cout << "Subscribing to " << std::to_string(senderID) << std::endl;
    
    return;
}

// Receiver Callback function for when the client receives a message from the server
void CorelinkInterface::corelinkRecvCallback(const int &receiverID, const int &senderID, const char *msg, const int &size)
{
    std::lock_guard<std::mutex> lck(printLock);
    
    //TODO: this shit
    /* Needs to write to JUCE accessible memory somehow here */
    juce::String recvString = juce::String(msg);
    juce::MemoryBlock memBlock = juce::MemoryBlock(&recvString, size);
    
    this->data.memoryBlock = new juce::MemoryBlock(memBlock);
}

void CorelinkInterface::attachStream(Corelink::SendStream stream)
{
    data.sender_stream = stream;
}

void CorelinkInterface::startSendRecvStreams()
{
    try {
        // Create our SenderStream, of type 'audio' in the 'Holodeck' workspace
        Corelink::SendStream sender_stream = Corelink::Client::createSender("Holodeck", "audio",
                                                                            "", true, true,
                                                                            Corelink::Const::STREAM_STATE_SEND_UDP);
        
        std::cout << Corelink::StreamData::getStreamData(STREAM_ID(sender_stream)) << std::endl;
        
        // attach it to our data struct within CorelinkInterface
        this->attachStream(sender_stream);
        
        // Print out current streams:
        std::cout <<"\n----Listing Streams: " << std::endl;
        std::vector<int> currStreamIDs = Corelink::Client::listStreams( { "Holodeck" }, { "audio" });
        for (int id : currStreamIDs) {
            std::cout << id << std::endl;
        }
        
        // Create our Receiver stream, and attach our callback function to it
        Corelink::RecvStream receiver_stream = Corelink::Client::createReceiver("Holodeck", {"audio", "receiver" },
                                                                                "", true, true,
                                                                                Corelink::Const::STREAM_STATE_RECV_UDP);
        
        std::cout << "Attaching callback function to receiver stream" << std::endl;
        receiver_stream.setOnRecieve(corelinkRecvCallback);
        
        std::cout << "Subscribing to receiver stream" << std::endl;
        Corelink::Client::subscribe(STREAM_ID(receiver_stream), currStreamIDs[0]);
        
        std::cout << "\n----STARTING CORELINK SENDER----" << std::endl;
    }
    catch (Corelink::CorelinkException error){
        std::cout << "CORELINK ERROR" << std::endl;
        std::cout << error.code << ": " << error.msg << std::endl;
    }
    catch (const char* msg) {
        std::cout << "MISC ERROR" << std::endl;
        std::cout << msg << std::endl;
    }
    catch (...) {
        std::cout << "OTHER ERROR" << std::endl;
    }
}


Data::Data(){
}
