//
// Created by fisp on 09.04.2026.
//

#pragma once
#include <cstdint>
#include <string>
#include "../commonFiles/resultFunc/ResultFunction.hpp"
#include <utility>
#include <variant>

enum class Operation: uint8_t {
    SEARCH_STATION = 'R',
    REGISTER_EU = 'A',
    SMS_SEND = 'M',
    DELIVERY_STATUS = 'm',
    HANDOVER = 'H',
    CONFIRM_USER = 'C',
    TEXT_MESSAGE = 't',
};

struct SearchStationData {
    std::string IMSI;
    int32_t position;
};

struct HandOverSuccessData {
    int32_t station;
};

struct RegisterUeData {
    std::string IMSI;
    std::string IMEI;
    std::string MSISDN;
    int32_t eNodeb_id;
};

struct SmsSendData {
    uint64_t TMSI_src;
    std::string MSISDN_dst;
    uint32_t SMS_ID;
    std::string text;
};

struct DeliveryStatusData {
    uint64_t TMSI_dst;
    uint32_t SMS_ID;
    bool status;
};

struct HandoverData {
    uint64_t TMSI;
    int32_t targetBsId;
};

struct SearchStationResponseData {
    int32_t station_id;
    double signalPower;
};

struct RegisterResponseData {
    uint64_t TMSI;
};

struct AuthData {
    uint64_t TMSI;
};

struct TextAnswer {
    std::string text;
};

using MessageData = std::variant<SearchStationData, RegisterUeData, SmsSendData,
                DeliveryStatusData, SearchStationResponseData, RegisterResponseData, AuthData,
                TextAnswer, HandoverData, HandOverSuccessData>;

class HandleMessage {
    char operation = 'N';
    MessageData data {SearchStationData{}};

    bool readSearchStationData(const std::string& bytes, size_t& start,SearchStationData& data);
    bool readRegisterUeData(const std::string& bytes, size_t& start, RegisterUeData& data);
    bool readDeliveryStatusData(const std::string& bytes, size_t& start, DeliveryStatusData& data);
    bool readSmsSendData(const std::string& bytes, size_t& start, SmsSendData& data);
    bool readHandoverData(const std::string& bytes, size_t& start, HandoverData& data);
public:
    HandleMessage() = default;
    HandleMessage(const char op, MessageData data): operation(op), data(std::move(data)) {};

    ResultStatus parserRawBytes(const std::string& rawBytes);
    std::string serializeMessageData();

    [[nodiscard]] char getOperation() const;
    [[nodiscard]] MessageData getData() const;
};
