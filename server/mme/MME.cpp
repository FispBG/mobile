//
// Created by fisp on 12.04.2026.
//

#include "MME.hpp"

#include "../commonFiles/resultFunc/ResultFunction.hpp"
#include "smsc/SMSC.hpp"

MME::MME(const int32_t mmeId, std::shared_ptr<Registration> regInterface)
    : mmeId(mmeId), registration(std::move(regInterface)) {}

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

void MME::registerStation(const int32_t bsId,
                          const std::shared_ptr<BaseStation>& station) {
  std::lock_guard lock(mutex);
  stations[bsId] = station;
}

uint64_t MME::generateTmsi(const std::string& imsi, const std::string& imei,
                           const int32_t bsId) {
  logger(RES_GOOD("Check user register in bd."));
  if (!registration->requestAuthInfo(imei, imsi)) {
    return 0;
  }

  logger(RES_GOOD("User registered in bd."));
  const uint64_t tmsi = static_cast<uint64_t>(mmeId) << 32 | nextTmsi++;
  std::lock_guard lock(mutex);

  pendingUsers[tmsi] = PendingUser{imsi, imei, bsId};
  return tmsi;
}

bool MME::confirmRegister(const std::string& imsi, const uint64_t tmsi,
                          const int32_t bsId) {
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

bool MME::submitSmsFromBs(
    const uint64_t tmsiSrc, const uint32_t smsId, const std::string& msisdnDst,
    const std::shared_ptr<BaseStation>& sourceStation) const {
  std::string msisdnSrc;
  if (!registration->getMsisdnByTmsi(tmsiSrc, msisdnSrc)) {
    return false;
  }

  logger(RES_GOOD("Create SMS context"));
  if (!smsc->createSmsContext(tmsiSrc, smsId, msisdnSrc, msisdnDst)) {
    return false;
  }

  logger(RES_GOOD("Station give text SMS"));
  std::string text;
  if (!sourceStation->takeTextFromSms(tmsiSrc, smsId, text)) {
    smsc->deleteSmsContext(smsId);
    return false;
  }

  logger(RES_GOOD("SMSC take text from station."));
  if (!smsc->takeSmsText(smsId, text, sourceStation->getId())) {
    smsc->deleteSmsContext(smsId);
    return false;
  }

  logger(RES_GOOD("Confirm take text to SMSC."));
  sourceStation->confirmTookText(tmsiSrc, smsId);

  return true;
}

bool MME::trySendSMS(const uint32_t smsId) {
  std::string msisdnSrc;
  std::string msisdnDst;
  std::string text;

  if (!smsc->getSmsForRetry(smsId, msisdnSrc, msisdnDst, text)) {
    return false;
  }

  logger(RES_GOOD("Find station for delivery."));
  uint64_t tmsiDst{};
  std::shared_ptr<BaseStation> destinationStation;
  if (!resolveSmsRoute(msisdnDst, tmsiDst, destinationStation)) {
    return false;
  }

  if (!destinationStation) {
    return false;
  }

  if (!destinationStation->MMEReserveBuffer(tmsiDst, smsId)) {
    return false;
  }

  logger(RES_GOOD("MME send sms from SMSC to dst station."));
  if (!destinationStation->MMESendTextSms(tmsiDst, smsId, msisdnSrc, text)) {
    return false;
  }

  smsc->markSmsTrySend(smsId);
  return true;
}

bool MME::resolveSmsRoute(const std::string& msisdnDst, uint64_t& tmsiDst,
                          std::shared_ptr<BaseStation>& destinationStation) {
  int32_t findBsId{};
  if (!registration->resolveDestination(msisdnDst, tmsiDst, findBsId)) {
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

void MME::notifySmsDelivery(const uint32_t smsId, const bool status) {
  uint64_t tmsiSrc{};
  logger(RES_GOOD("MME accept report about delivery."));
  if (!smsc->getSourceTmsi(smsId, tmsiSrc)) {
    return;
  }

  int32_t bsId{};
  if (!smsc->getSourceBsId(smsId, bsId)) {
    return;
  }

  std::shared_ptr<BaseStation> sourceStation;
  {
    std::lock_guard lock(stationsMutex);

    const auto findStation = stations.find(bsId);
    if (findStation == stations.end()) {
      return;
    }

    sourceStation = findStation->second.lock();
  }

  if (!sourceStation) {
    return;
  }

  logger(RES_GOOD("Notify SMS delivery to src."));
  sourceStation->sendDeliveryReportToUser(tmsiSrc, smsId, status);
}

void MME::ackSmsDeliveryReport(const uint32_t smsId) const {
  logger(RES_GOOD("Dst say, he get sms."));
  smsc->deleteSmsContext(smsId);
}

bool MME::changePathToUe(const uint64_t tmsi, const int32_t newBsId) const {
  logger(RES_GOOD("Change path to Ue."));
  if (!registration->changePathToUe(tmsi, newBsId)) {
    return false;
  }

  return true;
}
