//
// Created by fisp on 07.04.2026.
//

#pragma once

#include <string>

#define RES_ERROR(msg) ResultStatus{Status::Error, msg, __LINE__, __FILE__}
#define RES_WARNING(msg) ResultStatus{Status::Warning, msg, __LINE__, __FILE__}
#define RES_GOOD(msg) ResultStatus{Status::Good, msg, 0, ""}

namespace LogConfig {
    inline const std::string pathToLog = "log.txt";
    inline constexpr char dataFormat[] = "%Y-%m-%d %H:%M:%S";
}

enum class Status {
    Good,
    Error,
    Warning,
    None
};

struct InfoResult {
    Status condition = Status::None;
    std::string message;
    int64_t line {0};
    std::string file;
};

class ResultStatus {
    InfoResult infoResult {};

public:
    ResultStatus(const Status &status, std::string message, const int64_t line, std::string file):
    infoResult{status, std::move(message), line, std::move(file)} {};

    [[nodiscard]] bool isError() const;
    [[nodiscard]] bool isGood() const;
    [[nodiscard]] bool isNone() const;
    [[nodiscard]] bool isWarning() const;

    [[nodiscard]] const InfoResult& getInfoResult() const;
};

void logger(const ResultStatus &result);
