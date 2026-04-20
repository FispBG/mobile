//
// Created by fisp on 09.04.2026.
//

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

enum class Operation : uint8_t {
  SEARCH_STATION = 'R',
  REGISTER_EU = 'A',
  SMS_SEND = 'M',
  DELIVERY_STATUS = 'm',
  HANDOVER = 'H',
  CONFIRM_USER = 'C',
  TEXT_MESSAGE = 't',
};

struct SearchStationData {
  std::string imsi;
  int32_t position{};
};

struct HandOverSuccessData {
  int32_t station;
};

struct RegisterUeData {
  std::string imsi;
  std::string imei;
  std::string msisdn;
  int32_t stationId;
};

struct SmsSendData {
  uint64_t tmsiSrc;
  std::string msisdnDst;
  uint32_t smsId;
  std::string text;
};

struct DeliveryStatusData {
  uint64_t tmsiDst;
  uint32_t smsId;
  bool status;
};

struct HandoverData {
  uint64_t tmsi;
  int32_t targetBsId;
};

struct SearchStationResponseData {
  int32_t stationId;
  float signalPower;
};

struct RegisterResponseData {
  uint64_t tmsi;
};

struct AuthData {
  uint64_t tmsi;
};

struct TextAnswer {
  std::string text;
};

using MessageData = std::variant<SearchStationData, RegisterUeData, SmsSendData,
                                 DeliveryStatusData, SearchStationResponseData,
                                 RegisterResponseData, AuthData, TextAnswer,
                                 HandoverData, HandOverSuccessData>;

class HandleMessage {
  char operation = 'N';
  MessageData data{SearchStationData{}};

  template <typename T, typename Reader>
  MessageData readPacketTemplate(bool& isPacket, Reader reader);

  MessageData readSearchStationData(const std::string& bytes, size_t& start, bool& isPacket);
  MessageData readRegisterUeData(const std::string& bytes, size_t& start, bool& isPacket);
  MessageData readDeliveryStatusData(const std::string& bytes, size_t& start, bool& isPacket);
  MessageData readSmsSendData(const std::string& bytes, size_t& start, bool& isPacket);
  MessageData readHandoverData(const std::string& bytes, size_t& start, bool& isPacket);
  MessageData readTextAnswerData(const std::string& rawBytes, size_t& start, bool& isPacket);
  MessageData readAuthData(const std::string& rawBytes, size_t& start, bool& isPacket);

  inline void serializeSearchStationData(std::string& rawBytes, const SearchStationData& data);
  inline void serializeSearchStationResponseData(std::string& rawBytes, const SearchStationResponseData& data);
  inline void serializeRegisterUeData(std::string& rawBytes, const RegisterUeData& data);
  inline void serializeSmsSendData(std::string& rawBytes, const SmsSendData& data);
  inline void serializeDeliveryStatusData(std::string& rawBytes, const DeliveryStatusData& data);
  inline void serializeAuthData(std::string& rawBytes, const AuthData& data);
  inline void serializeTextAnswer(std::string& rawBytes, const TextAnswer& data);
  inline void serializeHandoverData(std::string& rawBytes, const HandoverData& data);
  inline void serializeHandOverSuccessData(std::string& rawBytes, const HandOverSuccessData& data);

 public:
  HandleMessage() = default;
  HandleMessage(const char op, MessageData data)
      : operation(op), data(std::move(data)){};

  bool parserRawBytes(const std::string& rawBytes, size_t& readBytes);
  std::string serializeMessageData();

  [[nodiscard]] char getOperation() const;
  [[nodiscard]] MessageData getData() const;
};

template <typename T, typename Reader>
MessageData HandleMessage::readPacketTemplate(bool& isPacket, Reader reader) {
  T massage{};

  const bool checkNeedField = reader(massage);

  if (!checkNeedField) {
    return MessageData{};
  }

  isPacket = true;
  return massage;
}