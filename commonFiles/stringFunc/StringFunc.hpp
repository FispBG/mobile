//
// Created by fisp on 14.04.2026.
//

#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

inline std::string stringStrip(const std::string &str) {
    const uint64_t startText = str.find_first_not_of(" \t\n\r\f\v");

    if (startText == std::string::npos) {
        return "";
    }
    const uint64_t endText = str.find_last_not_of(" \t\n\r\f\v");

    return str.substr(startText, endText - startText + 1);
}

constexpr uint64_t hashString(const char* str) {
    uint64_t hash = 0;
    while (*str) {
        hash = hash * 31 + static_cast<int>(*str);
        str++;
    }
    return hash;
}

inline std::vector<std::string> split(const std::string &str, const char separator) {
    std::vector<std::string> stringVec;

    std::stringstream ss(str);
    std::string subStr;

    while (getline(ss, subStr, separator)) {
        stringVec.push_back(subStr);
    }

    return stringVec;
}

inline bool isIntNumber(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    size_t index = 0;
    if (text[0] == '-' || text[0] == '+') {
        index = 1;
    }

    if (index >= text.size()) {
        return false;
    }

    for (; index < text.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(text[index]))) {
            return false;
        }
    }

    return true;
}

inline std::string fixInputString(const std::string &str) {
    std::string newString;

    for (const char ch : str) {
        const uint8_t chValue = std::tolower(ch);

        if (std::isprint(chValue)) {
            newString.push_back(static_cast<char>(chValue));
        }
    }

    return stringStrip(newString);
}
