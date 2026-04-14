//
// Created by fisp on 07.04.2026.
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "../context/context.hpp"

struct SearchResultData {
    int32_t bsId {};
    double power {};
};

struct PacketData {
    char op {};
    int32_t stationId {};
    double signalPower {};
    uint64_t tmsi {};
    std::string msisdn {};
    uint32_t smsId {};
    std::string text {};
    bool status {};
    int32_t position {};
};

class Exchange {
    Context& context;

    std::string ipAddress;
    uint16_t port {};

    int socketClient {-1};
    std::atomic<bool> running {false};
    std::thread readThread;

    std::mutex socketMutex;
    std::mutex authMutex;
    std::condition_variable condition;
    std::optional<uint64_t> pendingTMSI {0};
    std::vector<SearchResultData> searchResults;

    std::atomic<uint32_t> nextSmsId {1};

    bool connectToServer();
    void closeConnection();
    bool sendRaw(const std::string& rawBytes);

    void readLoop();
    void handlePacket(const PacketData& packet);

    std::vector<SearchResultData> searchStations();

    static std::string makeHandover(uint64_t tmsi, int32_t stationId);
    static std::string makeSearch(const std::string& imsi, int32_t pos);
    static std::string makeRegister(const std::string& imsi, const std::string& imei,
                                    const std::string& msisdn, int32_t stationId);

    static std::string makeConfirm(uint64_t tmsi);
    static std::string makeSms(uint64_t tmsi_src, const std::string& msisdnDst,
                               uint32_t smsId, const std::string& text);

    static std::string makeDelivery(uint64_t tmsiDst, uint32_t smsId, bool status);
    static std::string makePositionUpdate(uint64_t tmsi, int32_t position);

    static bool tryParseOne(const std::string& buffer, PacketData& packet, size_t& consumed);

public:
    Exchange(Context& context, const std::string& ipAddress, uint16_t port);
    ~Exchange();

    bool activate();
    void deactivate();

    bool registration();
    bool move(int32_t coordinate);
    bool sendSms(const std::string& msisdn_dst, const std::string& text);

    void printStatus() const;
    void printInbox() const;
    void printOutbox() const;
    void printStations();
};