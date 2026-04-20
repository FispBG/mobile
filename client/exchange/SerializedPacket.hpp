//
// Created by fisp on 17.04.2026.
//

#pragma once

#include "SerializedPacket.hpp"
#include "../commonFiles/byteFunc/BytesTransform.hpp"

#include <cstdint>
#include <string>

inline std::string makeSearchStationPacket(const std::string& imsi, const int32_t pos) {
    std::string out;
    writeUint8(out, 'R');
    writeString(out, imsi);
    writeInt32(out, pos);
    return out;
}

inline std::string makeHandoverStationPacket(const uint64_t tmsi,
                                   const int32_t stationId) {
    std::string out;
    writeUint8(out, 'H');
    writeUint64(out, tmsi);
    writeInt32(out, stationId);
    return out;
}

inline std::string makeRegisterPacket(const std::string& imsi,
                                   const std::string& imei,
                                   const std::string& msisdn,
                                   const int32_t stationId) {
    std::string out;
    writeUint8(out, 'A');
    writeString(out, imsi);
    writeString(out, imei);
    writeString(out, msisdn);
    writeInt32(out, stationId);
    return out;
}

inline std::string makeConfirmRegisterPacket(const uint64_t tmsi) {
    std::string out;
    writeUint8(out, 'C');
    writeUint64(out, tmsi);
    return out;
}

inline std::string makeSmsSendPacket(const uint64_t tmsi_src,
                              const std::string& msisdnDst,
                              const uint32_t smsId, const std::string& text) {
    std::string out;
    writeUint8(out, 'M');
    writeUint64(out, tmsi_src);
    writeString(out, msisdnDst);
    writeUint32(out, smsId);
    writeString(out, text);
    return out;
}

inline std::string makeDeliveryReportPacket(const uint64_t tmsiDst, const uint32_t smsId,
                                   const bool status) {
    std::string out;
    writeUint8(out, 'm');
    writeUint64(out, tmsiDst);
    writeUint32(out, smsId);
    writeBool(out, status);
    return out;
}

inline std::string makePositionUpdatePacket(const uint64_t tmsi,
                                         const int32_t position) {
    std::string out;
    writeUint8(out, 'H');
    writeUint64(out, tmsi);
    writeInt32(out, position);
    return out;
}