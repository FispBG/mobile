//
// Created by fisp on 12.04.2026.
//

#include "MME.hpp"

MME::MME(const int32_t mmeId, std::shared_ptr<Registration> regInterface):
            mmeId(mmeId), registration(std::move(regInterface)) {}

void MME::start() {
    std::lock_guard lock(mutex);
    running.store(true);
}

void MME::stop() {
    std::lock_guard lock(mutex);
    running.store(false);
    pendingUsers.clear();
}

void MME::attachSmsc(const std::shared_ptr<SMSC>& smscInterface) {
    std::lock_guard lock(mutex);
    smsc = smscInterface;
}

void MME::registerStation(const int32_t bsId, const std::shared_ptr<BaseStation>& station) {
    std::lock_guard lock(mutex);
    stations[bsId] = station;
}

uint64_t MME::generateTMSI(const std::string& imsi, const std::string& imei, const int32_t bsId) {

    if (!registration->requestAuthInfo(imei, imsi)) {
        return 0;
    }

    const uint64_t tmsi = (static_cast<uint64_t>(static_cast<uint32_t>(mmeId_)) << 32) | nextTmsi_++;
    std::lock_guard lock(mutex);

    pendingUsers[tmsi] = PendingUser{imsi, imei, bsId};
    return tmsi;
}

bool MME::confirmRegister(const std::string& imsi, const std::string& msisdn,
                          const uint64_t tmsi, const int32_t bsId) {
    {
        std::lock_guard lock(mutex);

        const auto user = pendingUsers.find(tmsi);
        pendingUsers.erase(user);
    }

    if (!registration->changePathToUe(tmsi, bsId)) {
        return false;
    }

    if (!registration->updateLocation(tmsi, bsId, imsi)) {
        return false;
    }

    return true;
}

bool MME::submitSmsFromBs(const uint64_t tmsi_src, const uint32_t sms_id,
                          const std::string& msisdn_dst, const std::shared_ptr<BaseStation>& sourceStation) {

    std::string msisdn_src;
    if (!registration->getMsisdnByTmsi(tmsi_src, msisdn_src)) {
        return false;
    }

    if (!smsc->createSmsContext(tmsi_src, sms_id, msisdn_src, msisdn_dst)) {
        return false;
    }

    std::string text;
    if (!sourceStation->takeTextFromSms(tmsi_src, sms_id, text)) {
        smsc->deleteSmsContext(sms_id);
        return false;
    }

    if (!smsc->takeSmsText(sms_id, text, sourceStation)) {
        smsc->deleteSmsContext(sms_id);
        return false;
    }

    sourceStation->confirmTookText(tmsi_src, sms_id);

    uint64_t tmsi_dst {};
    std::shared_ptr<BaseStation> destinationStation;
    if (!resolveSmsRoute(tmsi_src, msisdn_dst, tmsi_dst, msisdn_src, destinationStation)) {
        return true;
    }

    if (!destinationStation || !destinationStation->MMEReserveBuffer(tmsi_dst, sms_id)) {
        return true;
    }

    std::string textSms;
    if (!smsc->getSmsText(sms_id, textSms)) {
        return true;
    }

    destinationStation->MMESendTextSms(tmsi_dst, sms_id, msisdn_src, textSms);
    return true;
}

bool MME::resolveSmsRoute(const uint64_t tmsi_src, const std::string& msisdn_dst,
                          uint64_t& tmsi_dst, std::string& msisdn_src,
                          std::shared_ptr<BaseStation>& destinationStation) {
    int32_t findBsId {};
    if (!registration->resolveDestination(msisdn_dst, tmsi_dst, msisdn_src, findBsId)) {
        return false;
    }

    std::lock_guard lock(stationsMutex);
    const auto station = stations.find(findBsId);

    destinationStation = station->second.lock();

    if (!destinationStation) {
        return false;
    }

    return true;
}

void MME::notifySmsDelivery(const uint32_t sms_id, const bool status) {
    uint64_t tmsi_src {};
    if (!smsc->getSourceTmsi(sms_id, tmsi_src)) {
        return;
    }

    int32_t bs_id {};
    if (!smsc->getServingBsId(tmsi_src, bs_id)) {
        return;
    }

    std::shared_ptr<BaseStation> sourceStation;
    {
        std::lock_guard lock(stationsMutex);

        const auto findStation = stations.find(bs_id);
        if (findStation == stations.end()) {
            return;
        }

        sourceStation = findStation->second.lock();
    }

    if (!sourceStation) {
        return;
    }

    sourceStation->sendDeliveryReportToUser(tmsi_src, sms_id, status);
}

void MME::ackSmsDeliveryReport(const uint32_t smsId) {
    smsc->ackDeliveryReport(smsId);
}

bool MME::changePathToUe(const uint64_t tmsi, const int32_t newBsId) {

    if (!registration->changePathToUe(tmsi, newBsId)) {
        return false;
    }

    return true;
}