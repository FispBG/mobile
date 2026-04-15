//
// Created by fisp on 12.04.2026.
//

#include "MME.hpp"

#include "smsc/SMSC.hpp"

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

void MME::setSmsc(const std::shared_ptr<SMSC>& smscInterface) {
    std::lock_guard lock(mutex);
    smsc = smscInterface;
}

void MME::registerStation(const int32_t bsId, const std::shared_ptr<BaseStation>& station) {
    std::lock_guard lock(mutex);
    stations[bsId] = station;
}

uint64_t MME::generateTMSI(const std::string& imsi, const std::string& imei, const int32_t bsId) {

    logger(RES_GOOD("Check user register in bd."));
    if (!registration->requestAuthInfo(imei, imsi)) {
        return 0;
    }

    logger(RES_GOOD("User registered in bd."));
    const uint64_t tmsi = (static_cast<uint64_t>(static_cast<uint32_t>(mmeId)) << 32) | nextTMSI++;
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
    logger(RES_GOOD("Update location."));
    if (!registration->updateLocation(tmsi, bsId, imsi)) {
        return false;
    }

    logger(RES_GOOD("Change path to Ue."));
    if (!registration->changePathToUe(tmsi, bsId)) {
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

    logger(RES_GOOD("Create SMS context"));
    if (!smsc->createSmsContext(tmsi_src, sms_id, msisdn_src, msisdn_dst)) {
        return false;
    }

    logger(RES_GOOD("Station give text SMS"));
    std::string text;
    if (!sourceStation->takeTextFromSms(tmsi_src, sms_id, text)) {
        smsc->deleteSmsContext(sms_id);
        return false;
    }

    logger(RES_GOOD("SMSC take text from station."));
    if (!smsc->takeSmsText(sms_id, text, sourceStation->getId())) {
        smsc->deleteSmsContext(sms_id);
        return false;
    }

    logger(RES_GOOD("Confirm take text to SMSC."));
    sourceStation->confirmTookText(tmsi_src, sms_id);

    return true;
}

bool MME::trySendSMS(const uint32_t smsId) {
    std::string msisdn_src;
    std::string msisdn_dst;
    std::string text;

    if (!smsc->getSmsForRetry(smsId, msisdn_src, msisdn_dst, text)) {
        return false;
    }

    logger(RES_GOOD("Find station for delivery."));
    uint64_t tmsi_dst {};
    std::shared_ptr<BaseStation> destinationStation;
    if (!resolveSmsRoute(msisdn_dst, tmsi_dst, destinationStation)) {
        return false;
    }

    if (!destinationStation) {
        return false;
    }

    if (!destinationStation->MMEReserveBuffer(tmsi_dst, smsId)) {
        return false;
    }

    logger(RES_GOOD("MME send sms from SMSC to dst station."));
    if (!destinationStation->MMESendTextSms(tmsi_dst, smsId, msisdn_src, text)) {
        return false;
    }

    smsc->markSmsTrySend(smsId);
    return true;
}

bool MME::resolveSmsRoute(const std::string& msisdn_dst, uint64_t& tmsi_dst, std::shared_ptr<BaseStation>& destinationStation) {
    int32_t findBsId {};
    if (!registration->resolveDestination(msisdn_dst, tmsi_dst, findBsId)) {
        return false;
    }

    std::lock_guard lock(stationsMutex);
    const auto station = stations.find(findBsId);
    if (station == stations.end()) {
        return false;
    }

    destinationStation = station->second.lock();

    return true;
}

void MME::notifySmsDelivery(const uint32_t sms_id, const bool status) {
    uint64_t tmsi_src {};
    logger(RES_GOOD("MME accept report about delivery."));
    if (!smsc->getSourceTmsi(sms_id, tmsi_src)) {
        return;
    }

    int32_t bs_id {};
    if (!smsc->getSourceBsId(sms_id, bs_id)) {
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

    logger(RES_GOOD("Notify SMS delivery to src."));
    sourceStation->sendDeliveryReportToUser(tmsi_src, sms_id, status);
}

void MME::ackSmsDeliveryReport(const uint32_t smsId) const {
    logger(RES_GOOD("Dst say, he get sms."));
    smsc->ackDeliveryReport(smsId);
}

bool MME::changePathToUe(const uint64_t tmsi, const int32_t newBsId) const {

    logger(RES_GOOD("Change path to Ue."));
    if (!registration->changePathToUe(tmsi, newBsId)) {
        return false;
    }

    return true;
}
