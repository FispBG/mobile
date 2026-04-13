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
        std::cout << operation << std::endl;
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
            default:
                continue;
        }

    }
}

double BaseStation::calculateSignalPower(const int32_t position) const {
    return 1.0 - (static_cast<float>(std::abs(config.position - position)) / config.radius);
}

void BaseStation::handleSearch(const SearchStationData& data,  const std::shared_ptr<UeContext>& sender) const {
    const double signalPower = calculateSignalPower(data.position);

    if (signalPower >= 0) {
        SearchStationResponseData response {config.bs_id, signalPower};
        HandleMessage message {'r', response};
        sender->sendToClient(message.serializeMessageData());
    }

}

int32_t BaseStation::getId() const {
    return config.bs_id;
}

void BaseStation::handleRegister(const RegisterUeData& data, const std::shared_ptr<UeContext>& sender) {
    uint64_t TMSI {};
    {
        std::lock_guard lock(mapMutex);
        if (connectedUsers.size() >= config.maxConnections) {
            return;
        }

        uint64_t TMSI = mmeObject->generateTMSI(data.IMSI, data.IMEI, config.bs_id);

        sender->setUserData(data.IMSI, data.MSISDN, data.IMEI);
        pendingUsers[TMSI] = sender;
    }
    AuthData authData {TMSI};
    HandleMessage message {'a', authData};
    sender->sendToClient(message.serializeMessageData());
}

void BaseStation::handleAuthConfirm(const AuthData& data, const std::shared_ptr<UeContext>& sender) {
    std::unique_lock lock(mapMutex);

    const auto user = pendingUsers.find(data.TMSI);

    mmeObject->confirmRegister(user->second->getIMSI(), user->second->getMSISDN(),data.TMSI, config.bs_id);

    sender->setTMSI(data.TMSI);
    sender->setBasestation(shared_from_this());

    connectedUsers[data.TMSI] = sender;
    userBuffers[data.TMSI] = UserBuffer {};

    pendingUsers.erase(user);

    TextAnswer textData { "Registered"};
    HandleMessage message {'t', textData};
    sender->sendToClient(message.serializeMessageData());
}

void BaseStation::handleSms(const SmsSendData& data, const std::shared_ptr<UeContext>& sender) {
    {
        std::lock_guard lock(mapMutex);

        if (!userBuffers.contains(data.TMSI_src) || !connectedUsers.contains(data.TMSI_src)) {
            return;
        }

        BufferedSMS sms;
        sms.TMSI_src = data.TMSI_src;
        sms.SMS_id = data.SMS_ID;
        sms.MSISDN_dst = data.MSISDN_dst;
        sms.text = data.text;

        mmeObject->submitSmsFromBs(data.TMSI_src, sms.SMS_id, data.MSISDN_dst, shared_from_this());

        userBuffers[data.TMSI_src].takeMessageFromUser[data.SMS_ID] = sms;
    }
}

bool BaseStation::takeTextFromSms(const uint64_t tmsi_src, const uint32_t sms_id, std::string& text) {
    std::lock_guard lock(mapMutex);

    const auto sms = userBuffers[tmsi_src].takeMessageFromUser[sms_id];

    text = sms.text;
    return true;
}

void BaseStation::confirmTookText(const uint64_t tmsi_src, const uint32_t sms_id) {
    std::lock_guard lock(mapMutex);

    userBuffers[tmsi_src].takeMessageFromUser.erase(sms_id);
}

bool BaseStation::MMEReserveBuffer(const uint64_t tmsi_dst, const uint32_t sms_id) {
    std::lock_guard lock(mapMutex);

    auto& buffer = userBuffers[tmsi_dst];

    BufferedSMS sms;
    sms.TMSI_dst = tmsi_dst;
    sms.SMS_id = sms_id;

    buffer.sendMessageToUser[sms_id] = std::move(sms);
    return true;
}

bool BaseStation::MMESendTextSms(const uint64_t tmsi_dst, const uint32_t sms_id,
                                    const std::string& msisdn_src, const std::string& text) {
    std::shared_ptr<UeContext> receiver;

    {
        std::lock_guard lock(mapMutex);

        const auto user = connectedUsers.find(tmsi_dst);
        receiver = user->second;

        const auto buffer = userBuffers.find(tmsi_dst);
        const auto sms = buffer->second.sendMessageToUser.find(sms_id);

        sms->second.TMSI_dst = tmsi_dst;
        sms->second.SMS_id = sms_id;
        sms->second.MSISDN_src = msisdn_src;
        sms->second.text = text;
    }

    SmsSendData deliverData {tmsi_dst, msisdn_src, sms_id, text};
    HandleMessage deliverMessage {'M', deliverData};
    receiver->sendToClient(deliverMessage.serializeMessageData());

    return true;
}



void BaseStation::handleDeliveryStatus(const DeliveryStatusData& data, const std::shared_ptr<UeContext>& sender) {
    {
        std::lock_guard lock(mapMutex);

        auto bufferIt = userBuffers.find(data.TMSI_dst);

        bufferIt->second.sendMessageToUser.erase(data.SMS_ID);
    }

    mmeObject->notifySmsDelivery(data.SMS_ID, data.status);
}

void BaseStation::sendDeliveryReportToUser(const uint64_t tmsiSrc, const uint32_t smsId, const bool status) {
    std::shared_ptr<UeContext> sender;

    {
        std::lock_guard lock(mapMutex);

        const auto userIt = connectedUsers.find(tmsiSrc);
        sender = userIt->second;
    }

    DeliveryStatusData report {tmsiSrc, smsId, status};
    HandleMessage message {'m', report};
    sender->sendToClient(message.serializeMessageData());

    mmeObject->ackSmsDeliveryReport(smsId);
}
