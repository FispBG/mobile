//
// Created by fisp on 07.04.2026.
//

#pragma once

#include "../context/Context.hpp"
#include "./ReadPacket.hpp"

#include <atomic>
#include <condition_variable>
#include <optional>

class ReadPacket;

struct SearchResultData {
  int32_t bsId{};
  double power{};
};

class Exchange {
  Context& context;

  std::string ipAddress;
  uint16_t port{};

  int socketClient{-1};
  std::atomic<bool> running{false};
  std::thread readThread;

  std::mutex socketMutex;
  std::mutex authMutex;
  std::condition_variable condition;
  std::optional<uint64_t> pendingTMSI{0};
  std::vector<SearchResultData> searchResults;

  std::atomic<uint32_t> nextSmsId{1};

  bool connectToServer();
  void closeConnection();
  bool sendRaw(const std::string& rawBytes);
  void readLoop();

  std::vector<SearchResultData> searchStations();
  inline void bestStationNow(SearchResultData& best);

  inline void handlePacket(const ReadPacket& packet);
  inline void proccessPowerSignalData(const MessageData& packet);
  inline void proccessRegisterResponseData(const MessageData& packet);
  inline void proccessHandOverSuccessData(const MessageData& packet);
  inline void proccessSmsAcceptData(const MessageData& packet);
  inline void proccessDeliveryStatusSmsData(const MessageData& packet);
  inline void proccessTextAnswerData(const MessageData& packet);

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