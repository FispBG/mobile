//
// Created by fisp on 18.04.2026.
//

#pragma once

#include <string>
#include <cstdint>

inline bool isPort(const std::string &port, uint16_t& answer) {
    if (port.empty()) {
        return false;
    }

    char* firstNotInt = nullptr;
    const long portValue = strtol(port.c_str(), &firstNotInt, 10);

    if (*firstNotInt != '\0') {
        return false;
    }

    if (portValue <= 65535 && portValue >= 0) {
        answer = static_cast<uint16_t>(portValue);
        return true;
    }
    return false;
}

inline bool isNumber(const std::string& number) {
    if (number.empty()) {
        return false;
    }

    size_t pos{0};
    if (number[0] == '-') {
        if (number.length() == 1) {
            return false;
        }
        pos = 1;
    }

    while (pos < number.length()) {
        if (!std::isdigit(number[pos])) {
            return false;
        }
        pos++;
    }

    return true;
}