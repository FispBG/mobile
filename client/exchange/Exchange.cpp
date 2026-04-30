//
// Created by fisp on 07.04.2026.
//

#include "./Exchange.hpp"
#include "../commonFiles/resultFunc/ResultFunction.hpp"
#include "./SerializedPacket.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

Exchange::Exchange(Context& context, const std::string& ipAddress,
                   const uint16_t port)
    : context(context), ipAddress(ipAddress), port(port) {}

Exchange::~Exchange() { deactivate(); }

bool Exchange::connectToServer() {
  std::lock_guard lock(socketMutex);

  if (socketClient != -1) {
    return true;
  }

  socketClient = socket(AF_INET, SOCK_STREAM, 0);
  if (socketClient < 0) {
    logger(RES_ERROR("Fail create socket."));
    socketClient = -1;
    return false;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0) {
    logger(RES_ERROR("Bad ip address."));
    close(socketClient);
    socketClient = -1;
    return false;
  }

  if (connect(socketClient, reinterpret_cast<sockaddr*>(&serverAddr),
              sizeof(serverAddr)) < 0) {
    logger(RES_ERROR("Fail connect."));
    close(socketClient);
    socketClient = -1;
    return false;
  }

  running.store(true);
  readThread = std::thread(&Exchange::readLoop, this);
  return true;
}

void Exchange::closeConnection() {
  running.store(false);

  {
    std::lock_guard lock(socketMutex);
    if (socketClient != -1) {
      shutdown(socketClient, SHUT_RDWR);
      close(socketClient);
      socketClient = -1;
    }
  }

  if (readThread.joinable()) {
    readThread.join();
  }
}

bool Exchange::sendRaw(const std::string& rawBytes) {
  std::lock_guard lock(socketMutex);

  if (socketClient == -1) {
    return false;
  }

  return send(socketClient, rawBytes.data(), rawBytes.size(), 0) >= 0;
}

bool Exchange::activate() {
  if (context.getActive() && context.getRegistered()) {
    std::cout << "UE already active." << std::endl;
    return true;
  }

  if (!connectToServer()) {
    std::cout << "Fail connect to server." << std::endl;
    return false;
  }

  context.setActive(true);
  return registration();
}

void Exchange::deactivate() {
  context.setActive(false);
  context.resetSession();
  closeConnection();
}

std::vector<SearchResultData> Exchange::searchStations() {
  {
    std::lock_guard lock(authMutex);
    searchResults.clear();
  }

  if (!sendRaw(makeSearchStationPacket(context.getDeviceSpec().imsi,
                                       context.getCoordinate()))) {
    return {};
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::lock_guard lock(authMutex);
  return searchResults;
}

void Exchange::bestStationNow(SearchResultData& best) {
  const auto stations = searchStations();
  if (stations.empty()) {
    std::cout << "No reachable eNode-B for pos=" << context.getCoordinate()
              << std::endl;
    return;
  }

  best = *std::ranges::max_element(stations,
                                   [](const auto& left, const auto& right) {
                                     return left.power < right.power;
                                   });
}

bool Exchange::registration() {
  context.resetSession();

  SearchResultData best;
  bestStationNow(best);
  if (best.bsId == -1) {
    return false;
  }

  context.setStationId(best.bsId);

  std::cout << "Best station: bs=" << best.bsId << " power=" << std::fixed
            << best.power << std::endl;

  {
    std::lock_guard lock(authMutex);
    pendingTMSI = 0;
  }

  if (!sendRaw(makeRegisterPacket(context.getDeviceSpec().imsi,
                                  context.getDeviceSpec().imei,
                                  context.getDeviceSpec().msisdn, best.bsId))) {
    return false;
  }

  std::unique_lock lock(authMutex);
  const bool gotTmsi = condition.wait_for(
      lock, std::chrono::seconds(2), [this]() { return pendingTMSI != 0; });

  if (!gotTmsi) {
    std::cout << "Register timeout or auth failed." << std::endl;
    return false;
  }

  const uint64_t tmsi = pendingTMSI.value();
  pendingTMSI = 0;
  lock.unlock();

  context.setTmsi(tmsi);

  if (!sendRaw(makeConfirmRegisterPacket(context.getTmsi()))) {
    return false;
  }

  context.setRegistered(true);

  std::cout << "Attached with TMSI=" << context.getTmsi() << std::endl;
  return true;
}

bool Exchange::move(const int32_t coordinate) {
  context.setCoordinate(coordinate);
  std::cout << "New coordinate: " << coordinate << std::endl;

  if (!context.getActive() || !context.getRegistered() ||
      context.getTmsi() == 0) {
    return true;
  }

  SearchResultData best;
  bestStationNow(best);

  if (best.bsId == -1) {
    return false;
  }

  if (best.bsId == context.getStationId()) {
    return true;
  }

  std::cout << "Handover request: " << context.getStationId() << " to "
            << best.bsId << std::endl;

  return sendRaw(makeHandoverStationPacket(context.getTmsi(), best.bsId));
}

bool Exchange::sendSms(const std::string& msisdn_dst, const std::string& text) {
  if (!context.getActive() || !context.getRegistered() ||
      context.getTmsi() == 0) {
    std::cout << "UE is not registered. Use ACTIVE ON first." << std::endl;
    return false;
  }

  SmsOutData sms{};
  sms.smsId = nextSmsId.fetch_add(1);
  sms.msisdnDst = msisdn_dst;
  sms.text = text;
  sms.status = "PENDING";
  context.addOutSms(sms);

  if (!sendRaw(
          makeSmsSendPacket(context.getTmsi(), msisdn_dst, sms.smsId, text))) {
    context.updateOutSmsStatus(sms.smsId, "SEND_ERROR");
    return false;
  }

  std::cout << "SMS send: id=" << sms.smsId << " dst=" << msisdn_dst
            << std::endl;
  return true;
}

void Exchange::readLoop() {
  std::string rawBytes;
  char buffer[4096];

  while (running.load()) {
    const ssize_t readBytes = recv(socketClient, buffer, sizeof(buffer), 0);
    if (readBytes <= 0) {
      break;
    }

    rawBytes.append(buffer, readBytes);

    while (true) {
      size_t readBytesCount{};
      ReadPacket readPacket;

      if (!readPacket.parserRawBytes(rawBytes, readBytesCount)) {
        break;
      }

      handlePacket(readPacket);
      rawBytes.erase(0, readBytesCount);
    }
  }

  running.store(false);
}

void Exchange::handlePacket(const ReadPacket& packet) {
  switch (packet.getOperation()) {
    case 'r': {
      proccessPowerSignalData(packet.getData());
      break;
    }
    case 'a': {
      proccessRegisterResponseData(packet.getData());
      break;
    }
    case 'M': {
      proccessSmsAcceptData(packet.getData());
      break;
    }
    case 'm': {
      proccessDeliveryStatusSmsData(packet.getData());
      break;
    }
    case 't': {
      proccessTextAnswerData(packet.getData());
      break;
    }
    case 'h': {
      proccessHandOverSuccessData(packet.getData());
      break;
    }
    default:
      break;
  }
}

void Exchange::proccessPowerSignalData(const MessageData& packet) {
  const auto data = std::get<PowerSignalData>(packet);

  std::lock_guard lock(authMutex);
  searchResults.push_back(SearchResultData{data.stationId, data.signalPower});
}

void Exchange::proccessRegisterResponseData(const MessageData& packet) {
  const auto data = std::get<RegisterResponseData>(packet);

  {
    std::lock_guard lock(authMutex);
    pendingTMSI = data.tmsi;
  }

  condition.notify_all();
  std::cout << "Auth response: TMSI=" << data.tmsi << std::endl;
}

void Exchange::proccessHandOverSuccessData(const MessageData& packet) {
  const auto data = std::get<HandOverSuccessData>(packet);

  context.setStationId(data.station);
}

void Exchange::proccessSmsAcceptData(const MessageData& packet) {
  const auto data = std::get<SmsAcceptData>(packet);

  if (data.tmsiSrc != context.getTmsi()) {
    return;
  }

  SmsInData sms{};
  sms.smsId = data.smsId;
  sms.msisdnSrc = data.msisdnDst;
  sms.text = data.text;
  context.addInSms(sms);

  std::cout << "\nSMS from " << sms.msisdnSrc << ": " << sms.text << std::flush;

  sendRaw(makeDeliveryReportPacket(context.getTmsi(), sms.smsId, true));
}

void Exchange::proccessDeliveryStatusSmsData(const MessageData& packet) {
  const auto data = std::get<DeliveryStatusSmsData>(packet);

  context.updateOutSmsStatus(data.smsId, data.status ? "DELIVERED" : "FAILED");

  std::cout << "\nSMS id=" << data.smsId
            << " status=" << (data.status ? "DELIVERED" : "FAILED/EXPIRED")
            << std::flush;
}

void Exchange::proccessTextAnswerData(const MessageData& packet) {
  const auto data = std::get<TextAnswerData>(packet);

  std::cout << "SERVER: " << data.text << std::endl;
}

void Exchange::printStatus() const {
  std::cout << "active=" << (context.getActive() ? "true" : "false")
            << " registered=" << (context.getRegistered() ? "true" : "false")
            << " pos=" << context.getCoordinate()
            << " station=" << context.getStationId()
            << " tmsi=" << context.getTmsi() << std::endl;
}

void Exchange::printInbox() const {
  const auto inbox = context.getInSms();

  if (inbox.empty()) {
    std::cout << "Inbox is empty" << std::endl;
    return;
  }

  for (const auto& sms : inbox) {
    std::cout << "id=" << sms.smsId << " from=" << sms.msisdnSrc
              << " text=" << sms.text << std::endl;
  }
}

void Exchange::printOutbox() const {
  const auto outbox = context.getOutSms();

  if (outbox.empty()) {
    std::cout << "Outbox is empty" << std::endl;
    return;
  }

  for (const auto& sms : outbox) {
    std::cout << "id=" << sms.smsId << " to=" << sms.msisdnDst
              << " status=" << sms.status << " text=" << sms.text << std::endl;
  }
}

void Exchange::printStations() {
  if (!connectToServer()) {
    return;
  }

  const auto stations = searchStations();
  if (stations.empty()) {
    std::cout << "no available stations" << std::endl;
    return;
  }

  std::cout << "stations:" << std::endl;
  for (const auto& station : stations) {
    std::cout << "  bs=" << station.bsId << " power=" << std::fixed
              << station.power << std::endl;
  }
}