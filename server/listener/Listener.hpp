//
// Created by fisp on 08.04.2026.
//

#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "../../commonFiles/resultFunc/ResultFunction.hpp"
#include <vector>

class BaseStation;
class UeContext;

class Listener {
    int serverSocket{-1};
    std::vector<std::shared_ptr<BaseStation>> stationsOnline;
    std::vector<std::shared_ptr<UeContext>> activeUsers;

    std::atomic<bool> running {false};
    std::thread thread;
    std::mutex mutex;

    void acceptLoop();
public:

    ResultStatus createServerSocket(uint16_t port, int32_t maxConnections);

    void runServer();
    void stopServer();
    ~Listener();

    void setStationsOnline(std::vector<std::shared_ptr<BaseStation>> stations);
};
