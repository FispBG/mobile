//
// Created by fisp on 13.04.2026.
//

#pragma once

#include "mme/MME.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct SmsContextData {
  uint32_t smsId{};
  uint64_t tmsiSrc{};
  std::string msisdnSrc;
  std::string msisdnDst;
  std::string text;
  int32_t sourceBsId{-1};
  bool hasText{false};
  bool delivered{false};
  bool sendToBs{false};
  std::chrono::steady_clock::time_point timeDelete;
};

class SMSC {
  std::weak_ptr<MME> mme;
  std::unordered_map<uint32_t, SmsContextData> sms;

  std::size_t maxSmsCount{256};
  std::chrono::milliseconds smsLive{std::chrono::seconds(20)};

  std::atomic<bool> running{false};
  std::thread aliveSmsThread;

  mutable std::mutex mutex;
  std::condition_variable condition;

  void aliveSmsLoop();
  std::vector<uint32_t> removeOldMessage();

 public:
  explicit SMSC(std::chrono::milliseconds smsTimeDelete);
  ~SMSC();

  void start();
  void stop();

  bool createSmsContext(uint64_t tmsiSrc, uint32_t smsId,
                        const std::string& msisdnSrc,
                        const std::string& msisdnDst);

  template <typename function>
  bool templateChangeData(uint32_t smsId, function func);

  bool takeSmsText(uint32_t smsId, const std::string& text, int32_t sourceBsId);
  bool getSmsText(uint32_t smsId, std::string& text);
  bool getSourceTmsi(uint32_t smsId, uint64_t& tmsiSrc);

  bool getSourceBsId(uint32_t smsId, int32_t& bsId);

  void deleteSmsContext(uint32_t smsId);

  bool getSmsForRetry(uint32_t smsId, std::string& msisdnSrc,
                      std::string& msisdnDst, std::string& text) const;

  std::vector<uint32_t> collectSmsToRetry() const;
  bool markDelivered(uint32_t smsId);

  void setMME(const std::shared_ptr<MME>& mmeObject);

  bool markSmsTrySend(uint32_t smsId);
};

template <typename function>
bool SMSC::templateChangeData(const uint32_t smsId, function func) {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    func(it);
    return true;
}
