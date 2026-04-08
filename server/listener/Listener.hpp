//
// Created by fisp on 08.04.2026.
//

#pragma once
#include <atomic>
#include <cstdint>
#include <thread>

#include "../../commonFiles/resultFunc/ResultFunction.hpp"
#include <vector>

class Listener {
    int serverSocket{-1};

    std::atomic<bool> running {false};
    std::thread thread;
    void acceptLoop() const;
public:

    ResultStatus createServerSocket(uint16_t port, size_t maxConnections);

    void runServer();
    void stopServer();

};
