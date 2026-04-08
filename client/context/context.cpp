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
    connectionSpec.coordinate.store(coordinate);
}

int32_t Context::getCoordinate() const {
    return connectionSpec.coordinate.load();
}

void Context::setENodeb_id(const int32_t eNodeb_id) {
    connectionSpec.eNodeb_id.store(eNodeb_id);
}

int32_t Context::getENodeb_id() const {
    return connectionSpec.eNodeb_id.load();
}