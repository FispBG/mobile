//
// Created by fisp on 07.04.2026.
//

#include "ResultFunction.hpp"

#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <thread>

void logger(const ResultStatus &result) {
    if (result.isGood()) {
        return;
    }
    const std::string typeLog = result.isError() ? "[Error]" : "[Warning]";

    const auto systemTime = std::chrono::system_clock::now();
    const auto normalTime = std::chrono::system_clock::to_time_t(systemTime);

    const auto threadId = std::this_thread::get_id();
    const InfoResult infoResult = result.getInfoResult();

    std::ostringstream logString;

    logString << std::put_time(std::localtime(&normalTime), "%Y-%m-%d %H:%M:%S")
    << " | " << typeLog
    << " | Thread: " << threadId
    << " | " << infoResult.message
    << " | " << infoResult.file
    << " | line: " << infoResult.line << std::endl;

    std::ofstream logFile("log.txt", std::ios_base::app);
    if (logFile.is_open()) {
        logFile << logString.str();
        logFile.close();
    }

    if (result.isError()) {
        std::cerr << infoResult.message << std::endl;
    }else {
        std::cout << infoResult.message << std::endl;
    }
}
