//
// Created by fisp on 10.04.2026.
//

#include "BytesTransform.hpp"

#include <endian.h>
#include <netinet/in.h>

bool readUint8(const std::string& bytes, size_t& start, uint8_t& variable) {
  return readRawBytes(bytes, start, variable);
}

bool readUint32(const std::string& bytes, size_t& start, uint32_t& variable) {
  if (!readRawBytes(bytes, start, variable)) {
    return false;
  }

  variable = ntohl(variable);
  return true;
}

bool readInt32(const std::string& bytes, size_t& start, int32_t& variable) {
  uint32_t temp{};
  if (!readUint32(bytes, start, temp)) {
    return false;
  }

  variable = static_cast<int32_t>(temp);
  return true;
}

bool readUint64(const std::string& bytes, size_t& start, uint64_t& variable) {
  if (!readRawBytes(bytes, start, variable)) {
    return false;
  }

  variable = be64toh(variable);
  return true;
}

bool readBool(const std::string& bytes, size_t& start, bool& variable) {
  uint8_t temp{};
  if (!readRawBytes(bytes, start, temp)) {
    return false;
  }

  variable = temp ? true : false;
  return true;
}

bool readString(const std::string& bytes, size_t& start,
                std::string& variable) {
  uint32_t size{};
  if (!readUint32(bytes, start, size)) {
    return false;
  }

  if (start + size > bytes.size()) {
    return false;
  }

  std::string text(bytes.data() + start, size);
  variable = std::move(text);
  start += size;
  return true;
}

bool readFloat(const std::string& bytes, size_t& start, float& variable) {
  uint32_t temp{};
  if (!readUint32(bytes, start, temp)) {
    return false;
  }

  std::memcpy(&variable, &temp, sizeof(float));
  return true;
}

void writeUint8(std::string& bytes, const uint8_t variable) {
  writeRawBytes(bytes, variable);
}

void writeUint32(std::string& bytes, const uint32_t variable) {
  uint32_t temp = htonl(variable);
  writeRawBytes(bytes, temp);
}

void writeUint64(std::string& bytes, const uint64_t variable) {
  uint64_t temp = htobe64(variable);
  writeRawBytes(bytes, temp);
}

void writeBool(std::string& bytes, const bool variable) {
  uint8_t temp = variable ? 1 : 0;
  writeRawBytes(bytes, temp);
}

void writeInt32(std::string& bytes, const int32_t variable) {
  const auto temp = static_cast<uint32_t>(variable);
  writeUint32(bytes, temp);
}

void writeFloat(std::string& bytes, const float variable) {
  uint32_t temp;
  std::memcpy(&temp, &variable, sizeof(float));
  writeUint32(bytes, temp);
}

void writeString(std::string& bytes, const std::string& variable) {
  const auto size = static_cast<uint32_t>(variable.size());
  writeUint32(bytes, size);
  bytes.append(variable);
}
