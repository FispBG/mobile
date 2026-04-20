//
// Created by fisp on 07.04.2026.
//

#include "Context.hpp"

const DeviceSpec &Context::getDeviceSpec() const { return deviceSpec; }

uint64_t Context::getTmsi() const { return connectionSpec.tmsi; };

void Context::setTmsi(const uint64_t tmsi) { connectionSpec.tmsi = tmsi; }

void Context::setActive(const bool active) { connectionSpec.inActive = active; }

bool Context::getActive() const { return connectionSpec.inActive; }

void Context::setCoordinate(const int32_t position) {
  connectionSpec.position = position;
}

int32_t Context::getCoordinate() const { return connectionSpec.position; }

void Context::setStationId(const int32_t stationId) {
  connectionSpec.stationId = stationId;
}

int32_t Context::getStationId() const { return connectionSpec.stationId; }

void Context::resetSession() {
  connectionSpec.tmsi = 0;
  connectionSpec.registered = false;
  connectionSpec.stationId = -1;
}

void Context::setRegistered(const bool registered) {
  connectionSpec.registered = registered;
}

bool Context::getRegistered() const { return connectionSpec.registered; }

void Context::addOutSms(const SmsOutData &sms) {
  std::lock_guard lock(smsMutex);
  smsOut.push_back(sms);
}

void Context::addInSms(const SmsInData &sms) {
  std::lock_guard lock(smsMutex);
  smsIn.push_back(sms);
}

void Context::updateOutSmsStatus(const uint32_t smsId,
                                 const std::string &status) {
  std::lock_guard lock(smsMutex);

  for (auto &sms : smsOut) {
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