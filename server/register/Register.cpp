//
// Created by Fisp on 13.04.2026.
//

#include "Register.hpp"

#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

Registration::~Registration() {
    stop();
}

bool Registration::start() {
    std::lock_guard lock(mutex);
    if (running.load()) {
        return false;
    }
    if (loadFromDisk()) {
        return false;
    }

    loadFromDisk();
    running.store(true);
    return true;
}

void Registration::stop() {
    std::lock_guard lock(mutex);
    if (!running.load()) {
        return;
    }
    saveToDisk();
    running.store(false);
}

bool Registration::addUsers(const std::string& imsi, const std::string& imei, const std::string& msisdn) {
    std::lock_guard lock(mutex);

    UeRecord& record = byImsi[imsi];
    record.imsi = imsi;
    record.imei = imei;
    record.msisdn = msisdn;

    rebuildConvertData();
    if (running.load()) {
        return saveToDisk();
    }
    return true;
}

bool Registration::requestAuthInfo(const std::string& imei, const std::string& imsi) {
    std::lock_guard lock(mutex);

    const auto it = byImsi.find(imsi);
    if (it == byImsi.end()) {
        return false;
    }

    return it->second.imei == imei;
}

bool Registration::changePathToUe(const uint64_t tmsi, const int32_t bsId) {
    std::lock_guard lock(mutex);

    const auto tmsiAndImsi = tmsiToImsi.find(tmsi);
    if (tmsiAndImsi == tmsiToImsi.end()) {
        return false;
    }

    const auto recordUe = byImsi.find(tmsiAndImsi->second);
    if (recordUe == byImsi.end()) {
        return false;
    }

    recordUe->second.BsId_src = bsId;
    saveToDisk();
    return true;
}

bool Registration::updateLocation(const uint64_t tmsi, const int32_t vlrId, const std::string& imsi) {
    std::lock_guard lock(mutex);

    const auto recordIt = byImsi.find(imsi);
    if (recordIt == byImsi.end()) {
        return false;
    }

    if (recordIt->second.tmsi != 0) {
        tmsiToImsi.erase(recordIt->second.tmsi);
    }

    recordIt->second.tmsi = tmsi;
    recordIt->second.vlrId = vlrId;
    if (recordIt->second.BsId_src < 0) {
        recordIt->second.BsId_src = vlrId;
    }

    tmsiToImsi[tmsi] = imsi;
    msisdnToImsi[recordIt->second.msisdn] = imsi;

    if (running.load()) {
        saveToDisk();
    }

    return true;
}

bool Registration::getMsisdnByTmsi(const uint64_t tmsi, std::string& msisdn) const {
    std::lock_guard lock(mutex);

    const auto mapIt = tmsiToImsi.find(tmsi);
    if (mapIt == tmsiToImsi.end()) {
        return false;
    }

    const auto recordIt = byImsi.find(mapIt->second);
    if (recordIt == byImsi.end()) {
        return false;
    }

    msisdn = recordIt->second.msisdn;
    return true;
}

bool Registration::resolveDestination(const std::string& msisdnDst, uint64_t& tmsi_dst, int32_t& bsId) const {
    std::lock_guard lock(mutex);

    const auto imsiIt = msisdnToImsi.find(msisdnDst);
    if (imsiIt == msisdnToImsi.end()) {
        return false;
    }

    const auto recordIt = byImsi.find(imsiIt->second);
    if (recordIt == byImsi.end()) {
        return false;
    }

    if (recordIt->second.tmsi == 0 || recordIt->second.BsId_src < 0) {
        return false;
    }

    tmsi_dst = recordIt->second.tmsi;
    bsId = recordIt->second.BsId_src;

    return true;
}

bool Registration::loadFromDisk() {
    byImsi.clear();
    tmsiToImsi.clear();
    msisdnToImsi.clear();

    std::ifstream input(storagePath);
    if (!input.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }


        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> parts;
        while (std::getline(ss, token, '|')) {
            parts.push_back(token);
        }

        if (parts.size() < 6) {
            continue;
        }

        UeRecord record;
        record.imsi = parts[0];
        record.imei = parts[1];
        record.msisdn = parts[2];
        record.tmsi = std::stoull(parts[3]);
        record.BsId_src = std::stoi(parts[4]);
        record.vlrId = std::stoi(parts[5]);

        byImsi[record.imsi] = std::move(record);
    }

    rebuildConvertData();
    return true;
}

bool Registration::saveToDisk() const {
    std::ofstream output(storagePath, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    for (const auto& record : byImsi) {
        output << record.second.imsi << '|'
               << record.second.imei << '|'
               << record.second.msisdn << '|'
               << record.second.tmsi << '|'
               << record.second.BsId_src << '|'
               << record.second.vlrId << '\n';
    }

    return true;
}

void Registration::rebuildConvertData() {
    tmsiToImsi.clear();
    msisdnToImsi.clear();

    for (const auto& [imsi, record] : byImsi) {
        msisdnToImsi[record.msisdn] = imsi;
        if (record.tmsi != 0) {
            tmsiToImsi[record.tmsi] = imsi;
        }
    }
}
