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
    const std::string ip;
    const uint16_t port;
};

struct ConnectionSpec {
    std::atomic<bool> IN_ACTIVE = false;
    std::atomic<int32_t> coordinate;
    std::atomic<uint64_t> TMSI {0};
    std::atomic<int32_t> eNodeb_id {-1};
};

struct KnowNumberSpec {
    const std::string name;
    const std::string MSISDN;
};

class Context {
    DeviceSpec deviceSpec;
    ConnectionSpec connectionSpec {};
    std::vector<DeviceSpec> addressBook;
public:

    explicit Context(std::string IMEI, std::string IMSI, std::string MSISDN, std::string ip, const uint16_t port, const int32_t coordinate):
    deviceSpec{std::move(IMEI), std::move(IMSI), std::move(MSISDN), std::move(ip), port} {
        connectionSpec.coordinate.store(coordinate);
    };

    const DeviceSpec& getDeviceSpec() const;

    uint64_t getTMSI() const;

    void setTMSI(uint64_t TMSI);

    void setActive(bool ACTIVE);

    bool getActive() const ;

    void setCoordinate(int32_t coordinate);

    int32_t getCoordinate() const;

    void setENodeb_id(int32_t eNodeb_id);

    int32_t getENodeb_id() const;
};