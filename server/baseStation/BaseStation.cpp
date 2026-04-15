//
// Created by fisp on 08.04.2026.
//

#include "BaseStation.hpp"

#include <iostream>
#include <utility>

#include "UeContext.hpp"
#include "mme/MME.hpp"

BaseStation::BaseStation(const int32_t bs_id, const int32_t position, const int32_t MME_id, const float radius,
                         const int32_t maxConnections, const int32_t bufferSizeMax, std::shared_ptr<MME> mme) {

    config.bs_id = bs_id;
    config.position = position;
    config.MME_id = MME_id;
    config.radius = radius;
    config.maxConnections = maxConnections;
    config.bufferSizeMax = bufferSizeMax;
    mmeObject = std::move(mme);
};

BaseStation::~BaseStation() {
    stop();
}

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

void BaseStation::addDataInBuffer(const HandleMessage &dataStruct, std::shared_ptr<UeContext> sender) {
    std::lock_guard lock(queueMutex);
    messageQueue.push({dataStruct, std::move(sender)});
    queueCondition.notify_one();
}

void BaseStation::processLoop() {
    while (running.load()) {
        Request request;
        {
            std::unique_lock lock(queueMutex);
            queueCondition.wait(lock, [this]() {
                return !messageQueue.empty() || !running.load();
            });

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
                handleDeliveryStatus(std::get<DeliveryStatusData>(data), request.sender);
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

double BaseStation::calculateSignalPower(const int32_t position) const {
    return 1.0 - (static_cast<float>(std::abs(config.position - position)) / config.radius);
}

void BaseStation::handleSearch(const SearchStationData& data,  const std::shared_ptr<UeContext>& sender) const {
    std::cout << "----------------------------------" << std::endl;
    const double signalPower = calculateSignalPower(data.position);

    if (signalPower >= 0) {
        SearchStationResponseData response {config.bs_id, signalPower};
        HandleMessage message {'r', response};
        sender->sendToClient(message.serializeMessageData());
    }

    logger(RES_GOOD("Send search response to Ue."));
}

int32_t BaseStation::getId() const {
    return config.bs_id;
}

void BaseStation::handleRegister(const RegisterUeData& data, const std::shared_ptr<UeContext>& sender) {
    std::cout << "----------------------------------" << std::endl;
    logger(RES_GOOD("Accept register Ue."));
    uint64_t TMSI {};
    {
        std::lock_guard lock(mapMutex);
        if (connectedUsers.size() >= config.maxConnections) {
            return;
        }

        logger(RES_GOOD("Accept MME generate TMSI."));

        TMSI = mmeObject->generateTMSI(data.IMSI, data.IMEI, config.bs_id);
        if (TMSI == 0) {
            TextAnswer text{"Auth failed"};
            HandleMessage msg{'t', text};
            sender->sendToClient(msg.serializeMessageData());
            return;
        }

        sender->setUserData(data.IMSI, data.MSISDN, data.IMEI);
        pendingUsers[TMSI] = sender;
    }

    AuthData authData {TMSI};
    HandleMessage message {'a', authData};
    sender->sendToClient(message.serializeMessageData());
    logger(RES_GOOD("Send TMSI to Ue. " + std::to_string(TMSI)));
}

void BaseStation::handleAuthConfirm(const AuthData& data, const std::shared_ptr<UeContext>& sender) {
    std::unique_lock lock(mapMutex);

    const auto user = pendingUsers.find(data.TMSI);
    if (user == pendingUsers.end()) {
        return;
    }
    logger(RES_GOOD("Ue confirm accepted TMSI."));
    if (!mmeObject->confirmRegister(user->second->getIMSI(), user->second->getMSISDN(),
                                data.TMSI, config.bs_id)) {
        pendingUsers.erase(user);
        return;
    }

    sender->setTMSI(data.TMSI);
    sender->setBasestation(shared_from_this());

    connectedUsers[data.TMSI] = sender;
    userBuffers[data.TMSI] = UserBuffer {};

    pendingUsers.erase(user);

    TextAnswer textData { "Registered"};
    HandleMessage message {'t', textData};
    sender->sendToClient(message.serializeMessageData());

    logger(RES_GOOD("Register tmsi response: " + std::to_string(data.TMSI)));
}

void BaseStation::handleSms(const SmsSendData& data, const std::shared_ptr<UeContext>& sender) {
    std::cout << "----------------------------------" << std::endl;
    BufferedSMS sms;
    sms.TMSI_src = data.TMSI_src;
    sms.SMS_id = data.SMS_ID;
    sms.MSISDN_dst = data.MSISDN_dst;
    sms.text = data.text;
    logger(RES_GOOD("Station accept SMS."));
    {
        std::lock_guard lock(mapMutex);

        if (!userBuffers.contains(data.TMSI_src) || !connectedUsers.contains(data.TMSI_src)) {
            return;
        }

        userBuffers[data.TMSI_src].takeMessageFromUser[data.SMS_ID] = sms;
    }
    logger(RES_GOOD("Station send request to MME."));
    if (!mmeObject->submitSmsFromBs(data.TMSI_src, sms.SMS_id, data.MSISDN_dst, shared_from_this())) {
        std::lock_guard lock(mapMutex);
        userBuffers[data.TMSI_src].takeMessageFromUser.erase(data.SMS_ID);

        TextAnswer text{"SMS submit failed"};
        HandleMessage msg{'t', text};
        sender->sendToClient(msg.serializeMessageData());
    }
}

bool BaseStation::takeTextFromSms(const uint64_t tmsi_src, const uint32_t sms_id, std::string& text) {
    std::lock_guard lock(mapMutex);

    const auto buffer = userBuffers.find(tmsi_src);
    const auto sms = buffer->second.takeMessageFromUser.find(sms_id);

    if (sms == buffer->second.takeMessageFromUser.end()) {
        return false;
    }

    text = sms->second.text;
    return true;
}

void BaseStation::confirmTookText(const uint64_t tmsi_src, const uint32_t sms_id) {
    std::lock_guard lock(mapMutex);

    userBuffers[tmsi_src].takeMessageFromUser.erase(sms_id);
}

bool BaseStation::MMEReserveBuffer(const uint64_t tmsi_dst, const uint32_t sms_id) {
    std::lock_guard lock(mapMutex);

    const auto buffer = userBuffers.find(tmsi_dst);
    if (buffer == userBuffers.end()) {
        return false;
    }

    BufferedSMS sms;
    sms.TMSI_dst = tmsi_dst;
    sms.SMS_id = sms_id;

    buffer->second.sendMessageToUser[sms_id] = std::move(sms);

    logger(RES_GOOD("MME reserve buffer for sms: " + std::to_string(sms_id)));
    return true;
}

bool BaseStation::MMESendTextSms(const uint64_t tmsi_dst, const uint32_t sms_id,
                                    const std::string& msisdn_src, const std::string& text) {
    std::shared_ptr<UeContext> receiver;

    {
        std::lock_guard lock(mapMutex);

        const auto user = connectedUsers.find(tmsi_dst);
        if (user == connectedUsers.end()) {
            return false;
        }

        receiver = user->second;

        const auto buffer = userBuffers.find(tmsi_dst);

        const auto sms = buffer->second.sendMessageToUser.find(sms_id);
        if (sms == buffer->second.sendMessageToUser.end()) {
            return false;
        }

        logger(RES_GOOD("MME give SMS to station dst."));
        sms->second.TMSI_dst = tmsi_dst;
        sms->second.SMS_id = sms_id;
        sms->second.MSISDN_src = msisdn_src;
        sms->second.text = text;
    }

    SmsSendData deliverData {tmsi_dst, msisdn_src, sms_id, text};
    HandleMessage deliverMessage {'M', deliverData};
    receiver->sendToClient(deliverMessage.serializeMessageData());

    logger(RES_GOOD("Station send sms to receiver."));
    return true;
}

void BaseStation::handleDeliveryStatus(const DeliveryStatusData& data, const std::shared_ptr<UeContext>& sender) {
    logger(RES_GOOD("Ue dst confirm take sms."));
    {
        std::lock_guard lock(mapMutex);

        const auto buffer = userBuffers.find(data.TMSI_dst);
        if (buffer == userBuffers.end()) {
            return;
        }

        buffer->second.sendMessageToUser.erase(data.SMS_ID);
    }

    mmeObject->notifySmsDelivery(data.SMS_ID, data.status);
}

void BaseStation::sendDeliveryReportToUser(const uint64_t tmsiSrc, const uint32_t smsId, const bool status) {
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
    DeliveryStatusData report {tmsiSrc, smsId, status};
    HandleMessage message {'m', report};
    sender->sendToClient(message.serializeMessageData());

    mmeObject->ackSmsDeliveryReport(smsId);
}

void BaseStation::removeInactiveUser(const std::shared_ptr<UeContext>& user) {
    std::lock_guard lock(mapMutex);

    for (auto elem = pendingUsers.begin(); elem != pendingUsers.end(); ) {
        if (elem->second == user) {
            elem = pendingUsers.erase(elem);
        } else {
            ++elem;
        }
    }

    for (auto elem = connectedUsers.begin(); elem != connectedUsers.end(); ) {
        if (elem->second == user) {
            userBuffers.erase(elem->first);
            elem = connectedUsers.erase(elem);
        } else {
            ++elem;
        }
    }
}

bool BaseStation::canAcceptHandover() {
    std::lock_guard lock(mapMutex);
    return connectedUsers.size() < config.maxConnections;
}

bool BaseStation::acceptHandover(const uint64_t tmsi, const std::shared_ptr<UeContext>& user, const UserBuffer &buffer) {
    std::lock_guard lock(mapMutex);

    if (connectedUsers.size() >= config.maxConnections) {
        return false;
    }

    connectedUsers[tmsi] = user;
    userBuffers[tmsi] = buffer;
    return true;
}

void BaseStation::handleHandover(const HandoverData& data, const std::shared_ptr<UeContext>& sender) {
    std::cout << "----------------------------------" << std::endl;
    logger(RES_GOOD("Station handle request handover."));
    std::shared_ptr<BaseStation> targetStation;
    UserBuffer movedBuffer;

    {
        std::lock_guard lock(mapMutex);

        const auto userIt = connectedUsers.find(data.TMSI);
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

void BaseStation::startHandOver(const HandoverData& data, const std::shared_ptr<BaseStation>& targetStation,
                                UserBuffer movedBuffer, const std::shared_ptr<UeContext>& sender) {
    {
        std::lock_guard lock(mapMutex);

        const auto buffer = userBuffers.find(data.TMSI);
        if (buffer == userBuffers.end()) {
            TextAnswer text{"Handover failed: no user buffer"};
            HandleMessage msg{'t', text};
            sender->sendToClient(msg.serializeMessageData());
            return;
        }

        movedBuffer = buffer->second;
        userBuffers.erase(buffer);
        connectedUsers.erase(data.TMSI);
    }

    logger(RES_GOOD("User data transfer."));
    if (!targetStation->acceptHandover(data.TMSI, sender, movedBuffer)) {
        std::lock_guard lock(mapMutex);
        connectedUsers[data.TMSI] = sender;
        userBuffers[data.TMSI] = std::move(movedBuffer);

        TextAnswer text{"Handover failed: target BS rejected"};
        HandleMessage msg{'t', text};
        sender->sendToClient(msg.serializeMessageData());
        return;
    }

    sender->setBasestation(targetStation);

    if (!mmeObject->changePathToUe(data.TMSI, data.targetBsId)) {
        TextAnswer text{"Handover warning: moved, but VLR path update failed"};
        HandleMessage msg{'t', text};
        sender->sendToClient(msg.serializeMessageData());
        return;
    }

    logger(RES_GOOD("Send to Ue, all good."));
    HandOverSuccessData answer {};
    answer.station = targetStation->getId();
    HandleMessage msg{'h', answer};

    sender->sendToClient(msg.serializeMessageData());
}