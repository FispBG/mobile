//
// Created by fisp on 07.04.2026.
//

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

struct DeviceSpec {
    const std::string IMEI;
    const std::string IMSI;
    const std::string MSISDN;
};

struct SmsInData {
    uint32_t smsId {};
    std::string msisdn_src;
    std::string text;
};

struct SmsOutData {
    uint32_t smsId {};
    std::string msisdn_dst;
    std::string text;
    std::string status;
};

struct ConnectionSpec {
    std::atomic<uint64_t> TMSI {0};
    std::atomic<bool> IN_ACTIVE {false};
    std::atomic<bool> REGISTERED {false};
    std::atomic<int32_t> position {0};
    std::atomic<int32_t> eNodeb_id {-1};
};

class Context {
    DeviceSpec deviceSpec;
    ConnectionSpec connectionSpec;

    std::mutex smsMutex;
    std::vector<SmsOutData> smsOut;
    std::vector<SmsInData> smsIn;
public:

    explicit Context(std::string msisdn, std::string imsi, std::string imei, const int32_t position)
    : deviceSpec{std::move(imei), std::move(imsi), std::move(msisdn)} {
        connectionSpec.position.store(position);
    }
    
    const DeviceSpec& getDeviceSpec() const;

    uint64_t getTMSI() const;
    void setTMSI(uint64_t TMSI);

    void setActive(bool ACTIVE);
    bool getActive() const ;

    void setCoordinate(int32_t coordinate);
    int32_t getCoordinate() const;

    void setENodeb_id(int32_t eNodeb_id);
    int32_t getENodeb_id() const;

    void resetSession();

    void setRegistered(bool REGISTERED);
    bool getRegistered() const;

    void addOutSms(const SmsOutData& sms);
    void addInSms(const SmsInData& sms);
    void updateOutSmsStatus(uint32_t smsId, const std::string& status);

    std::vector<SmsOutData> getOutSms();
    std::vector<SmsInData> getInSms();
};