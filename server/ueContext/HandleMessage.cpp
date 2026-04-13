//
// Created by fisp on 09.04.2026.
//

#include "HandleMessage.hpp"
#include "BytesTransform.hpp"

ResultStatus HandleMessage::parserRawBytes(const std::string &rawBytes) {
    size_t start {0};

    uint8_t flagByte;
    if (!readUint8(rawBytes, start, flagByte)) {
        return RES_ERROR("Fail read flag packet.");
    }

    const auto flagOperation = static_cast<Operation>(flagByte);

    switch (flagOperation) {
        case Operation::SEARCH_STATION: {
            SearchStationData searchStationData {};

            if (!readSearchStationData(rawBytes, start, searchStationData)) {
                return RES_ERROR("Fail read searchStationData.");
            }

            data = std::move(searchStationData);
            operation = 'R';
            return RES_GOOD("");
        }

        case Operation::REGISTER_EU: {
            RegisterUeData registerUeData {};

            if (!readRegisterUeData(rawBytes, start, registerUeData)) {
                return RES_ERROR("Fail read registerUeData.");
            }

            data = std::move(registerUeData);
            operation = 'A';
            return RES_GOOD("");
        }

        case Operation::SMS_SEND: {
            SmsSendData smsSendData {};

            if (!readSmsSendData(rawBytes, start, smsSendData)) {
                return RES_ERROR("Fail read smsSendData.");
            }

            data = std::move(smsSendData);
            operation = 'M';
            return RES_GOOD("");
        }

        case Operation::DELIVERY_STATUS: {
            DeliveryStatusData deliveryStatusData {};

            if (!readDeliveryStatusData(rawBytes, start, deliveryStatusData)) {
                return RES_ERROR("Fail read deliveryStatusData.");
            }

            data = deliveryStatusData;
            operation = 'm';
            return RES_GOOD("");
        }

        case Operation::CONFIRM_USER: {
            AuthData authData {};

            if (!readUint64(rawBytes, start, authData.TMSI) || rawBytes.size() != start) {
                return RES_ERROR("Fail read AuthData.");
            }

            data = authData;
            operation = 'C';
            return RES_GOOD("");
        }

        case Operation::TEXT_MESSAGE: {
            TextAnswer textAnswer {};

            if (!readString(rawBytes, start, textAnswer.text) || rawBytes.size() != start) {
                return RES_ERROR("Fail read TextAnswer.");
            }

            data = textAnswer;
            operation = 't';
            return RES_GOOD("");
        }

        default:
            return RES_ERROR("Unknown operation.");
    }
}

bool HandleMessage::readSearchStationData(const std::string &bytes, size_t& start, SearchStationData &dataStruct) {
    const bool checkNeedField = readString(bytes, start, dataStruct.IMSI) &&
            readInt32(bytes, start, dataStruct.position) &&
            bytes.size() == start;

    if (!checkNeedField) {
        return false;
    }

    return true;
}

bool HandleMessage::readRegisterUeData(const std::string &bytes, size_t& start, RegisterUeData &dataStruct) {
    const bool checkNeedField = readString(bytes, start, dataStruct.IMSI) &&
            readString(bytes, start, dataStruct.IMEI) &&
            readString(bytes, start, dataStruct.MSISDN) &&
            readInt32(bytes, start, dataStruct.eNodeb_id) &&
            bytes.size() == start;

    if (!checkNeedField) {
        return false;
    }

    return true;
}

bool HandleMessage::readDeliveryStatusData(const std::string &bytes, size_t& start, DeliveryStatusData &dataStruct) {
    const bool checkNeedField = readUint64(bytes, start, dataStruct.TMSI_dst) &&
            readUint32(bytes, start, dataStruct.SMS_ID) &&
            readBool(bytes, start, dataStruct.status) &&
            bytes.size() == start;

    if (!checkNeedField) {
        return false;
    }

    return true;
}

bool HandleMessage::readSmsSendData(const std::string &bytes, size_t& start, SmsSendData &dataStruct) {
    const bool checkNeedField = readUint64(bytes, start, dataStruct.TMSI_src) &&
            readString(bytes, start, dataStruct.MSISDN_dst) &&
            readUint32(bytes, start, dataStruct.SMS_ID) &&
            readString(bytes, start, dataStruct.text) &&
            bytes.size() == start;

    if (!checkNeedField) {
        return false;
    }

    return true;
}

std::string HandleMessage::serializeMessageData() {
    std::string rawBytes {};

    writeUint8(rawBytes, static_cast<uint8_t>(operation));

    switch (operation) {
        case 'R': {
            const auto& dataSet = std::get<SearchStationData>(data);
            writeString(rawBytes, dataSet.IMSI);
            writeInt32(rawBytes, dataSet.position);
            break;
        }
        case 'r': {
            const auto& dataSet = std::get<SearchStationResponseData>(data);
            writeInt32(rawBytes, dataSet.station_id);
            writeDouble(rawBytes, dataSet.signalPower);
            break;
        }
        case 'A': {
            const auto& dataSet = std::get<RegisterUeData>(data);
            writeString(rawBytes, dataSet.IMSI);
            writeString(rawBytes, dataSet.IMEI);
            writeString(rawBytes, dataSet.MSISDN);
            writeInt32(rawBytes, dataSet.eNodeb_id);
            break;
        }
        case 'M': {
            const auto& dataSet = std::get<SmsSendData>(data);
            writeUint64(rawBytes, dataSet.TMSI_src);
            writeString(rawBytes, dataSet.MSISDN_dst);
            writeUint32(rawBytes, dataSet.SMS_ID);
            writeString(rawBytes, dataSet.text);
            break;
        }
        case 'm': {
            const auto& dataSet = std::get<DeliveryStatusData>(data);
            writeUint64(rawBytes, dataSet.TMSI_dst);
            writeUint32(rawBytes, dataSet.SMS_ID);
            writeBool(rawBytes, dataSet.status);
            break;
        }
        case 'c': {
            const auto& dataSet = std::get<AuthData>(data);
            writeUint64(rawBytes, dataSet.TMSI);
            break;
        }
        case 't': {
            const auto& dataSet = std::get<TextAnswer>(data);
            writeString(rawBytes, dataSet.text);
            break;
        }
        case 'a': {
            const auto& dataSet = std::get<AuthData>(data);
            writeUint64(rawBytes, dataSet.TMSI);
            break;
        }
        default:
            break;
    }

    return rawBytes;
}

char HandleMessage::getOperation() const {
    return operation;
}

MessageData HandleMessage::getData() const {
    return data;
}
