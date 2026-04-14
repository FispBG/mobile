//
// Created by fisp on 13.04.2026.
//

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

struct UeRecord {
    std::string imsi;
    std::string imei;
    std::string msisdn;
    uint64_t tmsi {0};
    int32_t BsId_src {-1};
    int32_t vlrId {-1};
};

class Registration {
    std::string storagePath = "registration.db";

    std::unordered_map<std::string, UeRecord> byImsi;
    std::unordered_map<uint64_t, std::string> tmsiToImsi;
    std::unordered_map<std::string, std::string> msisdnToImsi;

    mutable std::mutex mutex;
    std::atomic<bool> running {false};

    bool loadFromDisk();
    bool saveToDisk() const;
    void rebuildConvertData();

public:
    ~Registration();

    bool start();
    void stop();

    bool addUsers(const std::string& imsi, const std::string& imei, const std::string& msisdn);

    bool requestAuthInfo(const std::string& imei, const std::string& imsi);

    bool changePathToUe(uint64_t tmsi, int32_t bsId);

    bool updateLocation(uint64_t tmsi, int32_t vlrId, const std::string& imsi);

    bool getMsisdnByTmsi(uint64_t tmsi, std::string& msisdn) const;

    bool resolveDestination(const std::string& msisdnDst, uint64_t& tmsiDst, int32_t& bsId) const;
};


