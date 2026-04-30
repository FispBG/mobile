//
// Created by fisp on 08.04.2026.
//

#pragma once

#include "ueContext/HandleMessage.hpp"

#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>

class BaseStation;
class UeContext;
class MME;

struct BasestationConfig {
  int32_t bsId{};
  int32_t position{};
  int32_t mmeId{};
  int32_t radius{};
  int32_t maxConnections{};
  int32_t bufferSizeMax{};
  uint8_t networkVersion{};
};

struct BufferedSMS {
  uint64_t tmsiSrc{};
  uint64_t tmsiDst{};
  uint32_t smsId{};
  std::string msisdnSrc;
  std::string msisdnDst;
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

class BaseStation : public std::enable_shared_from_this<BaseStation> {
  BasestationConfig config{};

  std::shared_ptr<MME> mmeObject;

  std::unordered_map<uint64_t, std::shared_ptr<UeContext>> connectedUsers;
  std::unordered_map<uint64_t, UserBuffer> userBuffers;

  std::queue<Request> messageQueue;

  std::atomic<bool> running{false};
  std::thread stationThread;

  std::mutex queueMutex;
  std::condition_variable queueCondition;

  std::mutex mapMutex;
  std::unordered_map<uint64_t, std::shared_ptr<UeContext>> pendingUsers;

  void processLoop();
  void handleSearch(const SearchStationData& data,
                    const std::shared_ptr<UeContext>& sender) const;
  void handleRegister(const RegisterUeData& data,
                      const std::shared_ptr<UeContext>& sender);
  void handleSms(const SmsSendData& data,
                 const std::shared_ptr<UeContext>& sender);
  void handleDeliveryStatus(const DeliveryStatusData& data);
  void handleAuthConfirm(const AuthData& data,
                         const std::shared_ptr<UeContext>& sender);
  void startHandOver(const HandoverData& data,
                     const std::shared_ptr<BaseStation>& targetStation,
                     UserBuffer movedBuffer,
                     const std::shared_ptr<UeContext>& sender);

 public:
  BaseStation(int32_t bsId, int32_t position, int32_t mmeId, int32_t radius,
              int32_t maxConnections, int32_t bufferSizeMax,
              std::shared_ptr<MME> mme);

  ~BaseStation();
  void start();
  void stop();

  void addDataInBuffer(const HandleMessage& dataStruct,
                       std::shared_ptr<UeContext> sender);
  float calculateSignalPower(int32_t position) const;
  int32_t getId() const;

  void sendDeliveryReportToUser(uint64_t tmsiSrc, uint32_t smsId, bool status);
  bool takeTextFromSms(uint64_t tmsiSrc, uint32_t smsId, std::string& text);
  void confirmTookText(uint64_t tmsiSrc, uint32_t smsId);
  bool MMEReserveBuffer(uint64_t tmsiDst, uint32_t smsId);
  bool MMESendTextSms(uint64_t tmsiDst, uint32_t smsId,
                      const std::string& msisdnSrc, const std::string& text);

  void removeInactiveUser(const std::shared_ptr<UeContext>& user);
  bool canAcceptHandover();
  bool acceptHandover(uint64_t tmsi, const std::shared_ptr<UeContext>& user,
                      const UserBuffer& buffer);
  void handleHandover(const HandoverData& data,
                      const std::shared_ptr<UeContext>& sender);
};
