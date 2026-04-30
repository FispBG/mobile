//
// Created by fisp on 17.04.2026.
//

#pragma once

#include "../context/Context.hpp"

#include <cstdint>
#include <string>
#include <variant>

enum class Operation : uint8_t {
    SEARCH_STATION = 'r',
    REGISTER_CONFIRM = 'a',
    DELIVERY_STATUS = 'm',
    SEND_SMS = 'M',
    TEXT_MESSAGE = 't',
    HANDOVER = 'h',
};

struct PowerSignalData {
    int32_t stationId;
    float signalPower;
};

struct RegisterResponseData {
    uint64_t tmsi;
};

struct HandOverSuccessData {
    int32_t station;
};

struct SmsAcceptData {
    uint64_t tmsiSrc;
    std::string msisdnDst;
    uint32_t smsId;
    std::string text;
};

struct DeliveryStatusSmsData {
    uint64_t tmsiDst;
    uint32_t smsId;
    bool status;
};

struct TextAnswerData {
    std::string text;
};

using MessageData = std::variant<PowerSignalData, HandOverSuccessData, SmsAcceptData,
    DeliveryStatusSmsData, TextAnswerData, RegisterResponseData>;


class ReadPacket {
    char operation = 'N';
    MessageData data {};

    template <typename T, typename Reader>
    MessageData readPacketTemplate(bool& isPacket, Reader reader);

    MessageData readPowerSignalData(const std::string& rawBytes, size_t& start, bool& isPacket);
    MessageData readRegisterResponseData(const std::string& rawBytes, size_t& start, bool& isPacket);
    MessageData readHandOverSuccessData(const std::string& rawBytes, size_t& start, bool& isPacket);
    MessageData readSmsAcceptData(const std::string& rawBytes, size_t& start, bool& isPacket);
    MessageData readDeliveryStatusSmsData(const std::string& rawBytes, size_t& start, bool& isPacket);
    MessageData readTextAnswer(const std::string& rawBytes, size_t& start,  bool& isPacket);

public:

    bool parserRawBytes(const std::string& rawBytes, size_t& readBytes);

    [[nodiscard]] char getOperation() const;
    [[nodiscard]] MessageData getData() const;
};

template <typename T, typename Reader>
MessageData ReadPacket::readPacketTemplate(bool& isPacket, Reader reader) {
    T massage{};

    const bool checkNeedField = reader(massage);

    if (!checkNeedField) {
        return MessageData{};
    }

    isPacket = true;
    return massage;
}
