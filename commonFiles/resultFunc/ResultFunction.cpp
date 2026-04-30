//
// Created by fisp on 07.04.2026.
//

#include "ResultFunction.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

void logger(const ResultStatus& result) {
  if (result.isGood()) {
    std::cout << result.getInfoResult().message << std::endl;
    return;
  }
  const std::string typeLog = result.isError() ? "[Error]" : "[Warning]";

  const auto systemTime = std::chrono::system_clock::now();
  const auto normalTime = std::chrono::system_clock::to_time_t(systemTime);

  const auto threadId = std::this_thread::get_id();
  const InfoResult& infoResult = result.getInfoResult();

  std::ostringstream logString;

  logString << std::put_time(std::localtime(&normalTime), LogConfig::dataFormat)
            << " | " << typeLog << " | Thread: " << threadId << " | "
            << infoResult.message << " | " << infoResult.file
            << " | line: " << infoResult.line << std::endl;

  std::ofstream logFile(LogConfig::pathToLog, std::ios_base::app);
  if (logFile.is_open()) {
    logFile << logString.str();
    logFile.close();
  }

  if (result.isError()) {
    std::cerr << infoResult.message << std::endl;
  } else {
    std::cout << infoResult.message << std::endl;
  }
}

[[nodiscard]] bool ResultStatus::isError() const {
  return infoResult.condition == Status::Error;
}

[[nodiscard]] bool ResultStatus::isGood() const {
  return infoResult.condition == Status::Good;
}

[[nodiscard]] bool ResultStatus::isNone() const {
  return infoResult.condition == Status::None;
}

[[nodiscard]] bool ResultStatus::isWarning() const {
  return infoResult.condition == Status::Warning;
}

[[nodiscard]] const InfoResult& ResultStatus::getInfoResult() const {
  return infoResult;
}
