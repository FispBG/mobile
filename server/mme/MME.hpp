//
// Created by fisp on 12.04.2026.
//

#pragma once

#include "./register/Register.hpp"
#include "./baseStation/BaseStation.hpp"

#include <memory>
#include <unordered_map>

class SMSC;

struct PendingUser {
  std::string imsi;
  std::string imei;
  int32_t stationId{-1};
};

class MME {
  uint32_t nextTmsi{0};
  uint32_t mmeId{0};
  std::shared_ptr<Registration> registration;
  std::shared_ptr<SMSC> smsc;

  std::unordered_map<int32_t, std::weak_ptr<BaseStation>> stations;
  std::unordered_map<uint64_t, PendingUser> pendingUsers;

  std::atomic<bool> running{false};
  std::mutex mutex;
  std::mutex stationsMutex;

 public:
  explicit MME(int32_t mmeId, std::shared_ptr<Registration> regInterface);

  void start();
  void stop();

  void setSmsc(const std::shared_ptr<SMSC>& smsc);
  void registerStation(int32_t bsId,
                       const std::shared_ptr<BaseStation>& station);

  uint64_t generateTmsi(const std::string& imsi, const std::string& imei,
                        int32_t bsId);
  bool confirmRegister(const std::string& imsi, uint64_t tmsi, int32_t bsId);
  bool submitSmsFromBs(uint64_t tmsiSrc, uint32_t smsId,
                       const std::string& msisdnDst,
                       const std::shared_ptr<BaseStation>& sourceStation) const;

  bool resolveSmsRoute(const std::string& msisdnDst, uint64_t& tmsiDst,
                       std::shared_ptr<BaseStation>& destinationStation);

  void ackSmsDeliveryReport(uint32_t smsId) const;
  void notifySmsDelivery(uint32_t smsId, bool status);
  bool changePathToUe(uint64_t tmsi, int32_t newBsId) const;

  bool trySendSMS(uint32_t smsId);
};
