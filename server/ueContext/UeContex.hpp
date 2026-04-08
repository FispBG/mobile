//
// Created by fisp on 08.04.2026.
//

#pragma once
#include "../baseStation/BaseStation.hpp"
#include <atomic>
#include <memory>
#include <string>

class UeContext {
    int clientSocket;
    std::string IMSI;
    std::string MSISDN;
    uint64_t TMSI {0};
    std::shared_ptr<BaseStation> station;
    std::atomic<bool> running {false};

public:

    explicit UeContext(const int socket): clientSocket(socket) {};
    void start();
    void readSocket();
    void stop();
};

