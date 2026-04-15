//
// Created by fisp on 13.04.2026.
//

#include "SMSC.hpp"

#include <nlohmann/json.hpp>

SMSC::SMSC(const std::chrono::milliseconds smsTimeDelete): smsLive(smsTimeDelete) {}

SMSC::~SMSC() {
    stop();
}

void SMSC::start() {
    running.store(true);

    aliveSmsThread = std::thread(&SMSC::aliveSmsLoop, this);
}

void SMSC::stop() {
    running.exchange(false);

    condition.notify_all();
    if (aliveSmsThread.joinable()) {
        aliveSmsThread.join();
    }
}

void SMSC::setMME(const std::shared_ptr<MME>& mmeObject) {
    std::lock_guard lock(mutex);
    mme = mmeObject;
}

bool SMSC::createSmsContext(const uint64_t tmsiSrc, const uint32_t smsId,
                        const std::string& msisdnSrc, const std::string& msisdnDst) {

    std::lock_guard lock(mutex);

    removeOldMessage();
    if (sms.size() >= maxSmsCount) {
        return false;
    }

    if (sms.contains(smsId)) {
        return false;
    }

    SmsContextData context;
    context.smsId = smsId;
    context.tmsiSrc = tmsiSrc;
    context.msisdnSrc = msisdnSrc;
    context.msisdnDst = msisdnDst;
    context.timeDelete = std::chrono::steady_clock::now() + smsLive;

    sms[smsId] = std::move(context);
    return true;
}

bool SMSC::takeSmsText(const uint32_t smsId, const std::string& text, const int32_t sourceBsId) {

    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    it->second.text = text;
    it->second.hasText = true;
    it->second.sourceBsId = sourceBsId;
    it->second.timeDelete = std::chrono::steady_clock::now() + smsLive;
    return true;
}

bool SMSC::getSmsText(const uint32_t smsId, std::string& text) const {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end() || !it->second.hasText) {
        return false;
    }

    text = it->second.text;
    return true;
}

bool SMSC::getSourceTmsi(const uint32_t smsId, uint64_t& tmsiSrc) const {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    tmsiSrc = it->second.tmsiSrc;
    return true;
}

bool SMSC::getSourceBsId(const uint32_t smsId, int32_t& bsId) const {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    bsId = it->second.sourceBsId;
    return bsId >= 0;
}

void SMSC::ackDeliveryReport(const uint32_t smsId) {
    std::lock_guard lock(mutex);
    logger(RES_GOOD("Delete SMS context."));
    sms.erase(smsId);
}

void SMSC::deleteSmsContext(const uint32_t smsId) {
    std::lock_guard lock(mutex);
    logger(RES_GOOD("Delete SMS context."));
    sms.erase(smsId);
}

bool SMSC::getSmsForRetry(const uint32_t smsId, std::string& msisdn_src, std::string& msisdn_dst, std::string& text) const {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end() || !it->second.hasText || it->second.sendToBs) {
        return false;
    }

    msisdn_src = it->second.msisdnSrc;
    msisdn_dst = it->second.msisdnDst;
    text = it->second.text;
    return true;
}

void SMSC::aliveSmsLoop() {
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        std::vector<uint32_t> smsToDelete;
        std::vector<uint32_t> smsToRetry;
        {
            std::unique_lock lock(mutex);
            smsToDelete = std::move(removeOldMessage());
            smsToRetry = collectSmsToRetry();
        }

        const auto mmeObject = mme.lock();
        for (const auto& smsId : smsToRetry) {
            const bool deliveredToBs = mmeObject->trySendSMS(smsId);

            auto lostTime = sms[smsId].timeDelete - std::chrono::steady_clock::now();
            auto toSeconds = std::chrono::duration_cast<std::chrono::seconds>(lostTime).count();

            logger(RES_GOOD("SMSC retry send sms=" + std::to_string(smsId) +
                (deliveredToBs ? " success" : " fail") + "time: " +
                std::to_string(toSeconds)));
        }

        for (const auto& smsId : smsToDelete) {
            mmeObject->notifySmsDelivery(smsId, false);
        }
    }
}

bool SMSC::markDelivered(const uint32_t smsId) {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    it->second.delivered = true;
    return true;
}

std::vector<uint32_t> SMSC::collectSmsToRetry() const {
    std::vector<uint32_t> smsToRetry;

    for (const auto& [smsId, data] : sms) {
        if (data.hasText && !data.delivered) {
            smsToRetry.push_back(smsId);
        }
    }

    return smsToRetry;
}

std::vector<uint32_t> SMSC::removeOldMessage() {
    const auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> smsToDelete;

    for (auto elem = sms.begin(); elem != sms.end(); ) {
        if (elem->second.timeDelete <= now) {
            smsToDelete.push_back(elem->first);
            elem = sms.erase(elem);
        } else {
            ++elem;
        }
    }

    return smsToDelete;
}

bool SMSC::markSmsTrySend(const uint32_t smsId) {
    std::lock_guard lock(mutex);

    const auto it = sms.find(smsId);
    if (it == sms.end()) {
        return false;
    }

    it->second.sendToBs = true;
    return true;
}