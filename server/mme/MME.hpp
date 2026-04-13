//
// Created by fisp on 12.04.2026.
//

#pragma once

#include <memory>
#include <unordered_map>

#include "BaseStation.hpp"
#include "./smsc/SMSC.hpp"

class Registration;

struct PendingUser {
    std::string imsi;
    std::string imei;
    int32_t stationId {-1};
};

class MME {
    int32_t mmeId {0};
    std::shared_ptr<Registration> registration;
    std::shared_ptr<SMSC> smsc;

    std::unordered_map<int32_t, std::weak_ptr<BaseStation>> stations;
    std::unordered_map<uint64_t, PendingUser> pendingUsers;

    std::atomic<bool> running {false};
    std::mutex mutex;
    std::mutex stationsMutex;

public:

    explicit MME(int32_t mmeId, std::shared_ptr<Registration> regInterface);

    void start();
    void stop();

    void attachSmsc(const std::shared_ptr<SMSC>& smsc);
    void registerStation(int32_t bsId, const std::shared_ptr<BaseStation>& station);

    uint64_t generateTMSI(const std::string& imsi, const std::string& imei, int32_t bsId);
    bool confirmRegister(const std::string& imsi, const std::string& msisdn, uint64_t tmsi, int32_t bsId);
    bool submitSmsFromBs(uint64_t tmsi_src, uint32_t sms_id,
                        const std::string& msisdn_dst, const std::shared_ptr<BaseStation>& sourceStation);

    bool resolveSmsRoute(uint64_t tmsi_src, const std::string& msisdn_dst,
                          uint64_t& tmsi_dst, std::string& msisdn_src,
                          std::shared_ptr<BaseStation>& destinationStation);

    void ackSmsDeliveryReport(uint32_t smsId);
    void notifySmsDelivery(uint32_t sms_id, bool status);
    bool changePathToUe(uint64_t tmsi, int32_t newBsId);
};
