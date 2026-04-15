//
// Created by fisp on 07.04.2026.
//

#include "context.hpp"

const DeviceSpec& Context::getDeviceSpec() const {
    return deviceSpec;
}

uint64_t Context::getTMSI() const {
    return connectionSpec.TMSI.load();
};

void Context::setTMSI(const uint64_t TMSI) {
    connectionSpec.TMSI.store(TMSI);
}

void Context::setActive(const bool ACTIVE) {
    connectionSpec.IN_ACTIVE.store(ACTIVE);
}

bool Context::getActive() const {
    return connectionSpec.IN_ACTIVE.load();
}

void Context::setCoordinate(const int32_t coordinate) {
    connectionSpec.position.store(coordinate);
}

int32_t Context::getCoordinate() const {
    return connectionSpec.position.load();
}

void Context::setENodeb_id(const int32_t eNodeb_id) {
    connectionSpec.eNodeb_id.store(eNodeb_id);
}

int32_t Context::getENodeb_id() const {
    return connectionSpec.eNodeb_id.load();
}

void Context::resetSession() {
    connectionSpec.TMSI.store(0);
    connectionSpec.REGISTERED.store(false);
    connectionSpec.eNodeb_id.store(-1);
}

void Context::setRegistered(const bool REGISTERED) {
    connectionSpec.REGISTERED.store(REGISTERED);
}

bool Context::getRegistered() const {
    return connectionSpec.REGISTERED.load();
}

void Context::addOutSms(const SmsOutData& sms) {
    std::lock_guard lock(smsMutex);
    smsOut.push_back(sms);
}

void Context::addInSms(const SmsInData& sms) {
    std::lock_guard lock(smsMutex);
    smsIn.push_back(sms);
}

void Context::updateOutSmsStatus(const uint32_t smsId, const std::string& status) {
    std::lock_guard lock(smsMutex);

    for (auto& sms : smsOut) {
        if (sms.smsId == smsId) {
            sms.status = status;
            return;
        }
    }
}

std::vector<SmsOutData> Context::getOutSms() {
    std::lock_guard lock(smsMutex);
    return smsOut;
}

std::vector<SmsInData> Context::getInSms() {
    std::lock_guard lock(smsMutex);
    return smsIn;
}