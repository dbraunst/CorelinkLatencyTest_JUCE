/*
  ==============================================================================

    CorelinkInterface.h
    Created: 13 Jul 2021 10:02:01pm
    Author:  Daniel Braunstein

  ==============================================================================
*/

#pragma once

#include "Corelink.h"

#include <thread>
#include <chrono>
#include <iostream>

struct Data {
    Data();
    Corelink::SendStream sender_stream = Corelink::Client::createSender("Audiotest", "audiotest",
                                                                        "",
                                                                        true,
                                                                        true,
                                                                        Corelink::Const::STREAM_STATE_SEND_UDP);
};

class CorelinkInterface{
public:
    
    bool Connect();
    bool InitLoop();
    
    void openCorelink();
    void closeCorelink();
    
    static void corelinkServerUpdate(const int& receiverID, const int& senderID);
    static void corelinkRecvCallback(const int& receiverID, const int& senderID, const char* msg, const int& size);
    
    void attachStream(Corelink::SendStream);
    void startSendRecvStreams();
    
    Data data = Data();
private:
//    std::mutex printLock;
    std::string tmp1, tmp2;
};

