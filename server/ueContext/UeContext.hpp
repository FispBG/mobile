//
// Created by fisp on 08.04.2026.
//

#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class HandleMessage;
class BaseStation;

struct UserData {
    std::string IMSI;
    std::string MSISDN;
    std::string IMEI;
    uint64_t TMSI {0};
};

class UeContext : public std::enable_shared_from_this<UeContext> {
    int clientSocket {-1};

    UserData ueData {};

    std::vector<std::shared_ptr<BaseStation>> stationsOnline;
    std::shared_ptr<BaseStation> chooseStation;

    std::thread listenerThread;
    std::atomic<bool> running {false};

    std::mutex socketMutex;
public:

    explicit UeContext(int socket, std::vector<std::shared_ptr<BaseStation>> stations);
    ~UeContext();

    void start();
    void readSocket();
    void stop();

    void sendToBasestation(const HandleMessage& dataStruct);
    void sendToClient(const std::string& message);
    void bsRequestToDeleteInactive(const std::shared_ptr<UeContext>& user) const;

    void setUserData(const std::string& IMSI, const std::string& MSISDN, const std::string& IMEI);
    void setTMSI(uint64_t TMSI);
    std::string  getIMSI() const;
    void setBasestation(const std::shared_ptr<BaseStation>& station);
    std::string getMSISDN() const;
    bool isRunning() const;

    std::vector<std::shared_ptr<BaseStation>> getStationsOnline() const;
};