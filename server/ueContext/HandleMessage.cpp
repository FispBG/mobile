//
// Created by fisp on 09.04.2026.
//

#include "HandleMessage.hpp"
#include "../../commonFiles/byteFunc/BytesTransform.hpp"

bool HandleMessage::parserRawBytes(const std::string& rawBytes,
                                   size_t& readBytes) {
  readBytes = 0;
  size_t start{0};
  bool isPacket{false};

  uint8_t topByte;
  if (!readUint8(rawBytes, start, topByte)) {
    return false;
  }

  operation = static_cast<char>(topByte);
  const auto flagOperation = static_cast<Operation>(topByte);

  switch (flagOperation) {
    case Operation::SEARCH_STATION: {
      data = std::move(readSearchStationData(rawBytes, start, isPacket));
      break;
    }
    case Operation::REGISTER_EU: {
      data = std::move(readRegisterUeData(rawBytes, start, isPacket));
      break;
    }
    case Operation::SMS_SEND: {
      data = std::move(readSmsSendData(rawBytes, start, isPacket));
      break;
    }
    case Operation::DELIVERY_STATUS: {
      data = std::move(readDeliveryStatusData(rawBytes, start, isPacket));
      break;
    }
    case Operation::CONFIRM_USER: {
      data = std::move(readAuthData(rawBytes, start, isPacket));
      break;
    }
    case Operation::TEXT_MESSAGE: {
      data = std::move(readTextAnswerData(rawBytes, start, isPacket));
      break;
    }
    case Operation::HANDOVER: {
      data = std::move(readHandoverData(rawBytes, start, isPacket));
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

MessageData HandleMessage::readAuthData(const std::string& rawBytes,
                                        size_t& start, bool& isPacket) {
  return readPacketTemplate<AuthData>(isPacket, [&](AuthData& data) {
    return readUint64(rawBytes, start, data.tmsi);
  });
}

MessageData HandleMessage::readTextAnswerData(const std::string& rawBytes,
                                              size_t& start, bool& isPacket) {
  return readPacketTemplate<TextAnswer>(isPacket, [&](TextAnswer& data) {
    return readString(rawBytes, start, data.text);
  });
}

MessageData HandleMessage::readSearchStationData(const std::string& rawBytes,
                                                 size_t& start,
                                                 bool& isPacket) {
  return readPacketTemplate<SearchStationData>(
      isPacket, [&](SearchStationData& data) {
        return readString(rawBytes, start, data.imsi) &&
               readInt32(rawBytes, start, data.position);
      });
}

MessageData HandleMessage::readRegisterUeData(const std::string& rawBytes,
                                              size_t& start, bool& isPacket) {
  return readPacketTemplate<RegisterUeData>(
      isPacket, [&](RegisterUeData& data) {
        return readString(rawBytes, start, data.imsi) &&
               readString(rawBytes, start, data.imei) &&
               readString(rawBytes, start, data.msisdn) &&
               readInt32(rawBytes, start, data.stationId);
      });
}

MessageData HandleMessage::readHandoverData(const std::string& rawBytes,
                                            size_t& start, bool& isPacket) {
  return readPacketTemplate<HandoverData>(isPacket, [&](HandoverData& data) {
    return readUint64(rawBytes, start, data.tmsi) &&
           readInt32(rawBytes, start, data.targetBsId);
  });
}

MessageData HandleMessage::readDeliveryStatusData(const std::string& rawBytes,
                                                  size_t& start,
                                                  bool& isPacket) {
  return readPacketTemplate<DeliveryStatusData>(
      isPacket, [&](DeliveryStatusData& data) {
        return readUint64(rawBytes, start, data.tmsiDst) &&
               readUint32(rawBytes, start, data.smsId) &&
               readBool(rawBytes, start, data.status);
      });
}

MessageData HandleMessage::readSmsSendData(const std::string& rawBytes,
                                           size_t& start, bool& isPacket) {
  return readPacketTemplate<SmsSendData>(isPacket, [&](SmsSendData& data) {
    return readUint64(rawBytes, start, data.tmsiSrc) &&
           readString(rawBytes, start, data.msisdnDst) &&
           readUint32(rawBytes, start, data.smsId) &&
           readString(rawBytes, start, data.text);
  });
}

std::string HandleMessage::serializeMessageData() {
  std::string rawBytes{};

  writeUint8(rawBytes, static_cast<uint8_t>(operation));

  switch (operation) {
    case 'R': {
      serializeSearchStationData(rawBytes, std::get<SearchStationData>(data));
      break;
    }
    case 'r': {
      serializeSearchStationResponseData(
          rawBytes, std::get<SearchStationResponseData>(data));
      break;
    }
    case 'A': {
      serializeRegisterUeData(rawBytes, std::get<RegisterUeData>(data));
      break;
    }
    case 'M': {
      serializeSmsSendData(rawBytes, std::get<SmsSendData>(data));
      break;
    }
    case 'm': {
      serializeDeliveryStatusData(rawBytes, std::get<DeliveryStatusData>(data));
      break;
    }
    case 'a':
    case 'c': {
      serializeAuthData(rawBytes, std::get<AuthData>(data));
      break;
    }
    case 't': {
      serializeTextAnswer(rawBytes, std::get<TextAnswer>(data));
      break;
    }
    case 'H': {
      serializeHandoverData(rawBytes, std::get<HandoverData>(data));
      break;
    }
    case 'h': {
      serializeHandOverSuccessData(rawBytes,
                                   std::get<HandOverSuccessData>(data));
      break;
    }
    default:
      break;
  }

  return rawBytes;
}

inline void HandleMessage::serializeSearchStationData(
    std::string& rawBytes, const SearchStationData& data) {
  writeString(rawBytes, data.imsi);
  writeInt32(rawBytes, data.position);
}

inline void HandleMessage::serializeSearchStationResponseData(
    std::string& rawBytes, const SearchStationResponseData& data) {
  writeInt32(rawBytes, data.stationId);
  writeFloat(rawBytes, data.signalPower);
}

inline void HandleMessage::serializeRegisterUeData(std::string& rawBytes,
                                                   const RegisterUeData& data) {
  writeString(rawBytes, data.imsi);
  writeString(rawBytes, data.imei);
  writeString(rawBytes, data.msisdn);
  writeInt32(rawBytes, data.stationId);
}

inline void HandleMessage::serializeSmsSendData(std::string& rawBytes,
                                                const SmsSendData& data) {
  writeUint64(rawBytes, data.tmsiSrc);
  writeString(rawBytes, data.msisdnDst);
  writeUint32(rawBytes, data.smsId);
  writeString(rawBytes, data.text);
}

inline void HandleMessage::serializeDeliveryStatusData(
    std::string& rawBytes, const DeliveryStatusData& data) {
  writeUint64(rawBytes, data.tmsiDst);
  writeUint32(rawBytes, data.smsId);
  writeBool(rawBytes, data.status);
}

inline void HandleMessage::serializeAuthData(std::string& rawBytes,
                                             const AuthData& data) {
  writeUint64(rawBytes, data.tmsi);
}

inline void HandleMessage::serializeTextAnswer(std::string& rawBytes,
                                               const TextAnswer& data) {
  writeString(rawBytes, data.text);
}

inline void HandleMessage::serializeHandoverData(std::string& rawBytes,
                                                 const HandoverData& data) {
  writeUint64(rawBytes, data.tmsi);
  writeInt32(rawBytes, data.targetBsId);
}

inline void HandleMessage::serializeHandOverSuccessData(
    std::string& rawBytes, const HandOverSuccessData& data) {
  writeInt32(rawBytes, data.station);
}

char HandleMessage::getOperation() const { return operation; }

MessageData HandleMessage::getData() const { return data; }
