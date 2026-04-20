//
// Created by fisp on 17.04.2026.
//

#include "ReadPacket.hpp"
#include "../commonFiles/byteFunc/BytesTransform.hpp"

#include <iostream>

bool ReadPacket::parserRawBytes(const std::string& rawBytes,
                                size_t& readBytes) {
  readBytes = 0;
  size_t start{0};
  bool isPacket{false};

  uint8_t topByte{};
  if (!readUint8(rawBytes, start, topByte)) {
    return false;
  }

  operation = static_cast<char>(topByte);
  const auto flagOperation = static_cast<Operation>(operation);

  switch (flagOperation) {
    case Operation::SEARCH_STATION: {
      data = std::move(readPowerSignalData(rawBytes, start, isPacket));
      break;
    }
    case Operation::REGISTER_CONFIRM: {
      data = std::move(readRegisterResponseData(rawBytes, start, isPacket));
      break;
    }
    case Operation::SEND_SMS: {
      data = std::move(readSmsAcceptData(rawBytes, start, isPacket));
      break;
    }
    case Operation::DELIVERY_STATUS: {
      data = std::move(readDeliveryStatusSmsData(rawBytes, start, isPacket));
      break;
    }
    case Operation::TEXT_MESSAGE: {
      data = std::move(readTextAnswer(rawBytes, start, isPacket));
      break;
    }
    case Operation::HANDOVER: {
      data = std::move(readHandOverSuccessData(rawBytes, start, isPacket));
      break;
    }
    default:
      return false;
  }

  if (!isPacket) {
    return false;
  }

  readBytes = start;
  return true;
}

MessageData ReadPacket::readPowerSignalData(const std::string& rawBytes,
                                            size_t& start, bool& isPacket) {
  return readPacketTemplate<PowerSignalData>(
      isPacket, [&](PowerSignalData& data) {
        return readInt32(rawBytes, start, data.stationId) &&
               readFloat(rawBytes, start, data.signalPower);
      });
}

MessageData ReadPacket::readRegisterResponseData(const std::string& rawBytes,
                                                 size_t& start,
                                                 bool& isPacket) {
  return readPacketTemplate<RegisterResponseData>(
      isPacket, [&](RegisterResponseData& data) {
        return readUint64(rawBytes, start, data.tmsi);
      });
}

MessageData ReadPacket::readHandOverSuccessData(const std::string& rawBytes,
                                                size_t& start, bool& isPacket) {
  return readPacketTemplate<HandOverSuccessData>(
      isPacket, [&](HandOverSuccessData& data) {
        return readInt32(rawBytes, start, data.station);
      });
}

MessageData ReadPacket::readSmsAcceptData(const std::string& rawBytes,
                                          size_t& start, bool& isPacket) {
  return readPacketTemplate<SmsAcceptData>(isPacket, [&](SmsAcceptData& data) {
    return readUint64(rawBytes, start, data.tmsiSrc) &&
           readString(rawBytes, start, data.msisdnDst) &&
           readUint32(rawBytes, start, data.smsId) &&
           readString(rawBytes, start, data.text);
  });
}

MessageData ReadPacket::readDeliveryStatusSmsData(const std::string& rawBytes,
                                                  size_t& start,
                                                  bool& isPacket) {
  return readPacketTemplate<DeliveryStatusSmsData>(
      isPacket, [&](DeliveryStatusSmsData& data) {
        return readUint64(rawBytes, start, data.tmsiDst) &&
               readUint32(rawBytes, start, data.smsId) &&
               readBool(rawBytes, start, data.status);
      });
}

MessageData ReadPacket::readTextAnswer(const std::string& rawBytes,
                                       size_t& start, bool& isPacket) {
  return readPacketTemplate<TextAnswerData>(
      isPacket, [&](TextAnswerData& data) {
        return readString(rawBytes, start, data.text);
      });
}

char ReadPacket::getOperation() const { return operation; }

MessageData ReadPacket::getData() const { return data; }
