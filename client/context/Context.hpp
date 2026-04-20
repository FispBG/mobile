//
// Created by fisp on 07.04.2026.
//

#pragma once

#include <mutex>
#include <string>
#include <vector>

struct DeviceSpec {
  const std::string imei;
  const std::string imsi;
  const std::string msisdn;
};

struct SmsInData {
  uint32_t smsId{};
  std::string msisdnSrc;
  std::string text;
};

struct SmsOutData {
  uint32_t smsId{};
  std::string msisdnDst;
  std::string text;
  std::string status;
};

struct ConnectionSpec {
  uint64_t tmsi{0};
  bool inActive{false};
  bool registered{false};
  int32_t position{0};
  int32_t stationId{-1};
};

class Context {
  DeviceSpec deviceSpec{};
  ConnectionSpec connectionSpec{};

  std::mutex smsMutex;
  std::vector<SmsOutData> smsOut;
  std::vector<SmsInData> smsIn;

 public:
  explicit Context(std::string msisdn, std::string imsi, std::string imei,
                   const int32_t position)
      : deviceSpec{std::move(imei), std::move(imsi), std::move(msisdn)} {
    connectionSpec.position = position;
  }

  const DeviceSpec &getDeviceSpec() const;

  uint64_t getTmsi() const;
  void setTmsi(uint64_t tmsi);

  void setActive(bool active);
  bool getActive() const;

  void setCoordinate(int32_t position);
  int32_t getCoordinate() const;

  void setStationId(int32_t stationId);
  int32_t getStationId() const;

  void resetSession();

  void setRegistered(bool registered);
  bool getRegistered() const;

  void addOutSms(const SmsOutData &sms);
  void addInSms(const SmsInData &sms);
  void updateOutSmsStatus(uint32_t smsId, const std::string &status);

  std::vector<SmsOutData> getOutSms();
  std::vector<SmsInData> getInSms();
};