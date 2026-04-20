//
// Created by fisp on 10.04.2026.
//

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

template <typename T>
bool readRawBytes(const std::string& bytes, size_t& start, T& variable) {
    const size_t size = sizeof(T);

    if (start + size > bytes.size()) {
        return false;
    }

    std::memcpy(&variable, bytes.data() + start, size);
    start += size;
    return true;
}

bool readUint8(const std::string& bytes, size_t& start, uint8_t& variable);
bool readUint32(const std::string& bytes, size_t& start, uint32_t& variable);
bool readUint64(const std::string& bytes, size_t& start, uint64_t& variable);
bool readInt32(const std::string& bytes, size_t& start, int32_t& variable);
bool readString(const std::string& bytes, size_t& start, std::string& variable);
bool readBool(const std::string& bytes, size_t& start, bool& variable);
bool readFloat(const std::string& bytes, size_t& start, float& variable);

template <typename T>
void writeRawBytes(std::string& bytes, T& variable) {
    const auto ptr = reinterpret_cast<const char*>(&variable);
    bytes.append(ptr, sizeof(variable));
}

void writeUint8(std::string& bytes, uint8_t variable);
void writeUint32(std::string& bytes, uint32_t variable);
void writeUint64(std::string& bytes, uint64_t variable);
void writeInt32(std::string& bytes, int32_t variable);
void writeString(std::string& bytes, const std::string& variable);
void writeBool(std::string& bytes, bool variable);
void writeFloat(std::string& bytes, float variable);