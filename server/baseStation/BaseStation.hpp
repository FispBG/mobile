//
// Created by fisp on 08.04.2026.
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "HandleMessage.hpp"

class BaseStation;
class UeContext;
class MME;

struct BasestationConfig {
    int32_t bs_id {};
    int32_t position {};
    int32_t MME_id {};
    uint8_t networkVersion {};
    float radius {};
    int32_t maxConnections {};
    int32_t bufferSizeMax {};
};

struct BufferedSMS {
    uint64_t TMSI_src {};
    uint64_t TMSI_dst {};
    std::string MSISDN_src;
    std::string MSISDN_dst;
    uint32_t SMS_id {};
    std::string text;
};

struct UserBuffer {
    std::unordered_map<uint32_t, BufferedSMS> takeMessageFromUser;
    std::unordered_map<uint32_t, BufferedSMS> sendMessageToUser;
};

struct Request {
    HandleMessage dataStruct;
    std::shared_ptr<UeContext> sender;
};

class BaseStation : public std::enable_shared_from_this<BaseStation>{
    BasestationConfig config {};

    std::shared_ptr<MME> mmeObject;

    std::unordered_map<uint64_t, std::shared_ptr<UeContext>> connectedUsers;
    std::unordered_map<uint64_t, UserBuffer> userBuffers;

    std::queue<Request> messageQueue;

    std::atomic<bool> running {false};
    std::thread stationThread;

    std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::mutex mapMutex;
    std::unordered_map<uint64_t, std::shared_ptr<UeContext>> pendingUsers;

    void processLoop();
    void handleSearch(const SearchStationData& data, const std::shared_ptr<UeContext>& sender) const;
    void handleRegister(const RegisterUeData& data, const std::shared_ptr<UeContext>& sender);
    void handleSms(const SmsSendData& data, const std::shared_ptr<UeContext>& sender);
    void handleDeliveryStatus(const DeliveryStatusData& data, const std::shared_ptr<UeContext>& sender);
    void handleAuthConfirm(const AuthData& data, const std::shared_ptr<UeContext>& sender);

public:

    BaseStation(int32_t bs_id, int32_t position, int32_t MME_id, float radius,
        int32_t maxConnections, int32_t bufferSizeMax, std::shared_ptr<MME> mme);

    ~BaseStation();
    void start();
    void stop();

    void addDataInBuffer(const HandleMessage &dataStruct, std::shared_ptr<UeContext> sender);
    double calculateSignalPower(int32_t position) const;
    int32_t getId() const;

    void sendDeliveryReportToUser(uint64_t tmsiSrc, uint32_t smsId, bool status);
    bool takeTextFromSms(uint64_t tmsi_src, uint32_t sms_id, std::string& text);
    void confirmTookText(uint64_t tmsi_src, uint32_t sms_id);
    bool MMEReserveBuffer(uint64_t tmsi_dst, uint32_t sms_id);
    bool MMESendTextSms(uint64_t tmsi_dst, uint32_t sms_id,
                                    const std::string& msisdn_src, const std::string& text);
};


