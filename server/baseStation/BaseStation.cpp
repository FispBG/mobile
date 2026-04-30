//
// Created by fisp on 08.04.2026.
//

#include "BaseStation.hpp"
#include "../commonFiles/resultFunc/ResultFunction.hpp"
#include "../commonFiles/utilityFunc/UtilityFunc.hpp"
#include "mme/MME.hpp"
#include "ueContext/UeContext.hpp"

#include <iostream>
#include <utility>

BaseStation::BaseStation(const int32_t bsId, const int32_t position,
                         const int32_t mmeId, const int32_t radius,
                         const int32_t maxConnections,
                         const int32_t bufferSizeMax,
                         std::shared_ptr<MME> mme) {
  config.bsId = bsId;
  config.position = position;
  config.mmeId = mmeId;
  config.radius = radius;
  config.maxConnections = maxConnections;
  config.bufferSizeMax = bufferSizeMax;
  mmeObject = std::move(mme);
};

BaseStation::~BaseStation() { stop(); }

void BaseStation::start() {
  if (!running.load()) {
    running.store(true);
    stationThread = std::thread(&BaseStation::processLoop, this);
  }
}

void BaseStation::stop() {
  running.store(false);
  queueCondition.notify_all();
  if (stationThread.joinable()) {
    stationThread.join();
  }
}

void BaseStation::addDataInBuffer(const HandleMessage& dataStruct,
                                  std::shared_ptr<UeContext> sender) {
  std::lock_guard lock(queueMutex);
  messageQueue.push({dataStruct, std::move(sender)});
  queueCondition.notify_one();
}

void BaseStation::processLoop() {
  while (running.load()) {
    Request request;
    {
      std::unique_lock lock(queueMutex);
      queueCondition.wait(
          lock, [this]() { return !messageQueue.empty() || !running.load(); });

      if (!running.load() && messageQueue.empty()) {
        return;
      }

      request = std::move(messageQueue.front());
      messageQueue.pop();
    }

    const auto data = request.dataStruct.getData();
    const char operation = request.dataStruct.getOperation();

    switch (operation) {
      case 'R':
        handleSearch(std::get<SearchStationData>(data), request.sender);
        break;
      case 'A':
        handleRegister(std::get<RegisterUeData>(data), request.sender);
        break;
      case 'M':
        handleSms(std::get<SmsSendData>(data), request.sender);
        break;
      case 'm':
        handleDeliveryStatus(std::get<DeliveryStatusData>(data));
        break;
      case 'C':
        handleAuthConfirm(std::get<AuthData>(data), request.sender);
        break;
      case 'H':
        handleHandover(std::get<HandoverData>(data), request.sender);
        break;
      default:
        continue;
    }
  }
}

float BaseStation::calculateSignalPower(const int32_t position) const {
  return 1.0f - static_cast<float>(std::abs(config.position - position)) /
                    static_cast<float>(config.radius);
}

void BaseStation::handleSearch(const SearchStationData& data,
                               const std::shared_ptr<UeContext>& sender) const {
  std::cout << "----------------------------------" << std::endl;
  const float signalPower = calculateSignalPower(data.position);

  if (signalPower >= 0) {
    SearchStationResponseData response{config.bsId, signalPower};
    HandleMessage message{'r', response};
    sender->sendToClient(message.serializeMessageData());
  }

  logger(RES_GOOD("Send search response to Ue."));
}

int32_t BaseStation::getId() const { return config.bsId; }

void BaseStation::handleRegister(const RegisterUeData& data,
                                 const std::shared_ptr<UeContext>& sender) {
  std::cout << "----------------------------------" << std::endl;
  logger(RES_GOOD("Accept register Ue."));
  uint64_t tmsi{};
  {
    std::lock_guard lock(mapMutex);
    if (connectedUsers.size() >= static_cast<size_t>(config.maxConnections)) {
      return;
    }

    logger(RES_GOOD("Accept MME generate TMSI."));

    tmsi = mmeObject->generateTmsi(data.imsi, data.imei, config.bsId);
    if (tmsi == 0) {
      TextAnswer text{"Auth failed"};
      HandleMessage msg{'t', text};
      sender->sendToClient(msg.serializeMessageData());
      return;
    }

    sender->setUserData(data.imsi, data.msisdn, data.imei);
    pendingUsers[tmsi] = sender;
  }

  AuthData authData{tmsi};
  HandleMessage message{'a', authData};
  sender->sendToClient(message.serializeMessageData());
  logger(RES_GOOD("Send TMSI to Ue. " + std::to_string(tmsi)));
}

void BaseStation::handleAuthConfirm(const AuthData& data,
                                    const std::shared_ptr<UeContext>& sender) {
  std::unordered_map<unsigned long, std::shared_ptr<UeContext>>::iterator user;
  {
    std::unique_lock lock(mapMutex);

    const auto findUser = pendingUsers.find(data.tmsi);
    if (findUser == pendingUsers.end()) {
      return;
    }
    user = findUser;
  }
  logger(RES_GOOD("Ue confirm accepted TMSI."));

  const bool answerMme = mmeObject->confirmRegister(user->second->getIMSI(),
                                                    data.tmsi, config.bsId);
  if (!answerMme) {
    std::unique_lock lock(mapMutex);
    pendingUsers.erase(user);
    return;
  }

  sender->setTMSI(data.tmsi);
  sender->setBasestation(shared_from_this());
  {
    std::unique_lock lock(mapMutex);
    connectedUsers[data.tmsi] = sender;
    userBuffers[data.tmsi] = UserBuffer{};

    pendingUsers.erase(user);
  }
  TextAnswer textData{"Registered"};
  HandleMessage message{'t', textData};
  sender->sendToClient(message.serializeMessageData());

  logger(RES_GOOD("Register tmsi response: " + std::to_string(data.tmsi)));
}

void BaseStation::handleSms(const SmsSendData& data,
                            const std::shared_ptr<UeContext>& sender) {
  std::cout << "----------------------------------" << std::endl;
  BufferedSMS sms;
  sms.tmsiSrc = data.tmsiSrc;
  sms.smsId = data.smsId;
  sms.msisdnDst = data.msisdnDst;
  sms.text = data.text;
  logger(RES_GOOD("Station accept SMS."));
  {
    std::lock_guard lock(mapMutex);

    if (!userBuffers.contains(data.tmsiSrc) ||
        !connectedUsers.contains(data.tmsiSrc)) {
      return;
    }

    userBuffers[data.tmsiSrc].takeMessageFromUser[data.smsId] = sms;
  }
  logger(RES_GOOD("Station send request to MME."));
  if (!mmeObject->submitSmsFromBs(data.tmsiSrc, sms.smsId, data.msisdnDst,
                                  shared_from_this())) {
    std::lock_guard lock(mapMutex);
    userBuffers[data.tmsiSrc].takeMessageFromUser.erase(data.smsId);

    TextAnswer text{"SMS submit failed"};
    HandleMessage msg{'t', text};
    sender->sendToClient(msg.serializeMessageData());
  }
}

bool BaseStation::takeTextFromSms(const uint64_t tmsiSrc, const uint32_t smsId,
                                  std::string& text) {
  std::lock_guard lock(mapMutex);

  const auto buffer = userBuffers.find(tmsiSrc);
  const auto sms = buffer->second.takeMessageFromUser.find(smsId);

  if (sms == buffer->second.takeMessageFromUser.end()) {
    return false;
  }

  text = sms->second.text;
  return true;
}

void BaseStation::confirmTookText(const uint64_t tmsiSrc,
                                  const uint32_t smsId) {
  std::lock_guard lock(mapMutex);

  userBuffers[tmsiSrc].takeMessageFromUser.erase(smsId);
}

bool BaseStation::MMEReserveBuffer(const uint64_t tmsiDst,
                                   const uint32_t smsId) {
  std::lock_guard lock(mapMutex);

  const auto buffer = userBuffers.find(tmsiDst);
  if (buffer == userBuffers.end()) {
    return false;
  }

  BufferedSMS sms;
  sms.tmsiDst = tmsiDst;
  sms.smsId = smsId;

  buffer->second.sendMessageToUser[smsId] = std::move(sms);

  logger(RES_GOOD("MME reserve buffer for sms: " + std::to_string(smsId)));
  return true;
}

bool BaseStation::MMESendTextSms(const uint64_t tmsiDst, const uint32_t smsId,
                                 const std::string& msisdnSrc,
                                 const std::string& text) {
  std::shared_ptr<UeContext> receiver;

  {
    std::lock_guard lock(mapMutex);

    const auto user = connectedUsers.find(tmsiDst);
    if (user == connectedUsers.end()) {
      return false;
    }

    receiver = user->second;

    const auto buffer = userBuffers.find(tmsiDst);

    const auto sms = buffer->second.sendMessageToUser.find(smsId);
    if (sms == buffer->second.sendMessageToUser.end()) {
      return false;
    }

    logger(RES_GOOD("MME give SMS to station dst."));
    sms->second.tmsiDst = tmsiDst;
    sms->second.smsId = smsId;
    sms->second.msisdnSrc = msisdnSrc;
    sms->second.text = text;
  }

  SmsSendData deliverData{tmsiDst, msisdnSrc, smsId, text};
  HandleMessage deliverMessage{'M', deliverData};
  receiver->sendToClient(deliverMessage.serializeMessageData());

  logger(RES_GOOD("Station send sms to receiver."));
  return true;
}

void BaseStation::handleDeliveryStatus(const DeliveryStatusData& data) {
  logger(RES_GOOD("Ue dst confirm take sms."));
  {
    std::lock_guard lock(mapMutex);

    const auto buffer = userBuffers.find(data.tmsiDst);
    if (buffer == userBuffers.end()) {
      return;
    }

    buffer->second.sendMessageToUser.erase(data.smsId);
  }

  mmeObject->notifySmsDelivery(data.smsId, data.status);
}

void BaseStation::sendDeliveryReportToUser(const uint64_t tmsiSrc,
                                           const uint32_t smsId,
                                           const bool status) {
  std::shared_ptr<UeContext> sender;

  {
    std::lock_guard lock(mapMutex);

    const auto user = connectedUsers.find(tmsiSrc);
    if (user == connectedUsers.end()) {
      return;
    }

    sender = user->second;
  }

  logger(RES_GOOD("Station send delivery report to src."));
  DeliveryStatusData report{tmsiSrc, smsId, status};
  HandleMessage message{'m', report};
  sender->sendToClient(message.serializeMessageData());

  mmeObject->ackSmsDeliveryReport(smsId);
}

void BaseStation::removeInactiveUser(const std::shared_ptr<UeContext>& user) {
  std::lock_guard lock(mapMutex);

  deleteOneFromUnordMap(
      pendingUsers, [&](const auto& elem) { return elem.second == user; },
      [](const auto&) {});

  deleteOneFromUnordMap(
      connectedUsers, [&](const auto& elem) { return elem.second == user; },
      [&](const auto& elem) { return userBuffers.erase(elem.first); });
}

bool BaseStation::canAcceptHandover() {
  std::lock_guard lock(mapMutex);
  return connectedUsers.size() < static_cast<size_t>(config.maxConnections);
}

bool BaseStation::acceptHandover(const uint64_t tmsi,
                                 const std::shared_ptr<UeContext>& user,
                                 const UserBuffer& buffer) {
  std::lock_guard lock(mapMutex);

  if (connectedUsers.size() >= static_cast<size_t>(config.maxConnections)) {
    return false;
  }

  connectedUsers[tmsi] = user;
  userBuffers[tmsi] = buffer;
  return true;
}

void BaseStation::handleHandover(const HandoverData& data,
                                 const std::shared_ptr<UeContext>& sender) {
  std::cout << "----------------------------------" << std::endl;
  logger(RES_GOOD("Station handle request handover."));
  std::shared_ptr<BaseStation> targetStation;
  UserBuffer movedBuffer;

  {
    std::lock_guard lock(mapMutex);

    const auto userIt = connectedUsers.find(data.tmsi);
    if (userIt == connectedUsers.end() || userIt->second != sender) {
      TextAnswer text{"Handover failed: user not attached to this BS"};
      HandleMessage msg{'t', text};
      sender->sendToClient(msg.serializeMessageData());
      return;
    }
  }

  for (const auto& station : sender->getStationsOnline()) {
    if (station && station->getId() == data.targetBsId) {
      targetStation = station;
      break;
    }
  }

  if (!targetStation || targetStation.get() == this) {
    TextAnswer text{"Handover failed: bad target BS"};
    HandleMessage msg{'t', text};
    sender->sendToClient(msg.serializeMessageData());
    return;
  }

  logger(RES_GOOD("Handover request(station 1 say station 2)."));
  if (!targetStation->canAcceptHandover()) {
    TextAnswer text{"Handover failed: target BS is full"};
    HandleMessage msg{'t', text};
    sender->sendToClient(msg.serializeMessageData());
    return;
  }
  logger(RES_GOOD("Start handover."));
  startHandOver(data, targetStation, movedBuffer, sender);
}

void BaseStation::startHandOver(
    const HandoverData& data, const std::shared_ptr<BaseStation>& targetStation,
    UserBuffer movedBuffer, const std::shared_ptr<UeContext>& sender) {
  {
    std::lock_guard lock(mapMutex);

    const auto buffer = userBuffers.find(data.tmsi);
    if (buffer == userBuffers.end()) {
      TextAnswer text{"Handover failed: no user buffer"};
      HandleMessage msg{'t', text};
      sender->sendToClient(msg.serializeMessageData());
      return;
    }

    movedBuffer = buffer->second;
    userBuffers.erase(buffer);
    connectedUsers.erase(data.tmsi);
  }

  logger(RES_GOOD("User data transfer."));
  if (!targetStation->acceptHandover(data.tmsi, sender, movedBuffer)) {
    std::lock_guard lock(mapMutex);
    connectedUsers[data.tmsi] = sender;
    userBuffers[data.tmsi] = std::move(movedBuffer);

    TextAnswer text{"Handover failed: target BS rejected"};
    HandleMessage msg{'t', text};
    sender->sendToClient(msg.serializeMessageData());
    return;
  }

  sender->setBasestation(targetStation);

  if (!mmeObject->changePathToUe(data.tmsi, data.targetBsId)) {
    TextAnswer text{"Handover warning: moved, but VLR path update failed"};
    HandleMessage msg{'t', text};
    sender->sendToClient(msg.serializeMessageData());
    return;
  }

  logger(RES_GOOD("Send to Ue, all good."));
  HandOverSuccessData answer{};
  answer.station = targetStation->getId();
  HandleMessage msg{'h', answer};

  sender->sendToClient(msg.serializeMessageData());
}