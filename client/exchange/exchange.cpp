//
// Created by fisp on 07.04.2026.
//

#include "exchange.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include "../commonFiles/resultFunc/ResultFunction.hpp"
#include "../commonFiles/byteFunc/BytesTransform.hpp"


Exchange::Exchange(Context& context, const std::string& ipAddress, const uint16_t port)
    : context(context), ipAddress(ipAddress), port(port) {}

Exchange::~Exchange() {
    deactivate();
}

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

    sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        logger(RES_ERROR("Bad ip address."));
        close(socketClient);
        socketClient = -1;
        return false;
    }

    if (connect(socketClient, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
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

std::string Exchange::makeSearch(const std::string& imsi, const int32_t pos) {
    std::string out;
    writeUint8(out,'R');
    writeString(out, imsi);
    writeInt32(out, pos);
    return out;
}

std::string Exchange::makeHandover(const uint64_t tmsi, const int32_t stationId) {
    std::string out;
    writeUint8(out, 'H');
    writeUint64(out, tmsi);
    writeInt32(out, stationId);
    return out;
}

std::string Exchange::makeRegister(const std::string& imsi, const std::string& imei,
                                   const std::string& msisdn, const int32_t stationId) {
    std::string out;
    writeUint8(out, 'A');
    writeString(out, imsi);
    writeString(out, imei);
    writeString(out, msisdn);
    writeInt32(out, stationId);
    return out;
}

std::string Exchange::makeConfirm(const uint64_t tmsi) {
    std::string out;
    writeUint8(out, 'C');
    writeUint64(out, tmsi);
    return out;
}

std::string Exchange::makeSms(const uint64_t tmsi_src, const std::string& msisdnDst,
                              const uint32_t smsId, const std::string& text) {
    std::string out;
    writeUint8(out, 'M');
    writeUint64(out, tmsi_src);
    writeString(out, msisdnDst);
    writeUint32(out, smsId);
    writeString(out, text);
    return out;
}

std::string Exchange::makeDelivery(const uint64_t tmsiDst, const uint32_t smsId, const bool status) {
    std::string out;
    writeUint8(out, 'm');
    writeUint64(out, tmsiDst);
    writeUint32(out, smsId);
    writeBool(out, status);
    return out;
}

std::string Exchange::makePositionUpdate(const uint64_t tmsi, const int32_t position) {
    std::string out;
    writeUint8(out, 'H');
    writeUint64(out, tmsi);
    writeInt32(out, position);
    return out;
}

bool Exchange::tryParseOne(const std::string& buffer, PacketData& packet, size_t& consumed) {
    consumed = 0;
    size_t pos = 0;

    uint8_t opByte {};
    if (!readUint8(buffer, pos, opByte)) {
        return false;
    }

    packet = PacketData {};
    packet.op = static_cast<char>(opByte);

    switch (packet.op) {
        case 'r': {
            if (!readInt32(buffer, pos, packet.stationId)) {
                return false;
            }
            if (!readDouble(buffer, pos, packet.signalPower)) {
                return false;
            }
            break;
        }

        case 'a':
        case 'C': {
            if (!readUint64(buffer, pos, packet.tmsi)) {
                return false;
            }
            break;
        }

        case 'M': {
            if (!readUint64(buffer, pos, packet.tmsi)) {
                return false;
            }
            if (!readString(buffer, pos, packet.msisdn)) {
                return false;
            }
            if (!readUint32(buffer, pos, packet.smsId)) {
                return false;
            }
            if (!readString(buffer, pos, packet.text)) {
                return false;
            }
            break;
        }

        case 'm': {
            if (!readUint64(buffer, pos, packet.tmsi)) {
                return false;
            }
            if (!readUint32(buffer, pos, packet.smsId)) {
                return false;
            }
            if (!readBool(buffer, pos, packet.status)) {
                return false;
            }
            break;
        }

        case 't': {
            if (!readString(buffer, pos, packet.text)) {
                return false;
            }
            break;
        }

        case 'h': {
            if (!readInt32(buffer, pos, packet.stationId)) {
                return false;
            }
            break;
        }

        default:
            return false;
    }

    consumed = pos;
    return true;
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

    if (!sendRaw(makeSearch(context.getDeviceSpec().IMSI, context.getCoordinate()))) {
        return {};
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::lock_guard lock(authMutex);
    return searchResults;
}

bool Exchange::registration() {
    context.resetSession();

    const auto stations = searchStations();
    if (stations.empty()) {
        std::cout << "No reachable eNode-B for pos=" << context.getCoordinate() << std::endl;
        return false;
    }

    const auto best = *std::ranges::max_element(
        stations,
        [](const auto& left, const auto& right) {
            return left.power < right.power;
        });

    context.setENodeb_id(best.bsId);

    std::cout << "Best station: bs=" << best.bsId
              << " power=" << std::fixed << best.power << std::endl;

    {
        std::lock_guard lock(authMutex);
        pendingTMSI = 0;
    }

    if (!sendRaw(makeRegister(context.getDeviceSpec().IMSI, context.getDeviceSpec().IMEI, context.getDeviceSpec().MSISDN, best.bsId))) {
        return false;
    }

    std::unique_lock lock(authMutex);
    const bool gotTmsi = condition.wait_for(lock, std::chrono::seconds(2), [this]() {
        return pendingTMSI != 0;
    });

    if (!gotTmsi) {
        std::cout << "Register timeout or auth failed." << std::endl;
        return false;
    }

    const uint64_t tmsi = pendingTMSI.value();
    pendingTMSI = 0;
    lock.unlock();

    context.setTMSI(tmsi);

    if (!sendRaw(makeConfirm(context.getTMSI()))) {
        return false;
    }

    context.setRegistered(true);

    std::cout << "Attached with TMSI=" << context.getTMSI() << std::endl;
    return true;
}

bool Exchange::move(const int32_t coordinate) {
    context.setCoordinate(coordinate);
    std::cout << "New coordinate: " << coordinate << std::endl;

    if (!context.getActive() || !context.getRegistered() || context.getTMSI() == 0) {
        return true;
    }

    const auto stations = searchStations();
    if (stations.empty()) {
        std::cout << "No reachable eNode-B after move." << std::endl;
        return false;
    }

    const auto best = *std::ranges::max_element(
        stations,
        [](const auto& left, const auto& right) {
            return left.power < right.power;
        });

    if (best.bsId == context.getENodeb_id()) {
        return true;
    }

    std::cout << "Handover request: " << context.getENodeb_id() << " to " << best.bsId << std::endl;

    return sendRaw(makeHandover(context.getTMSI(), best.bsId));
}

bool Exchange::sendSms(const std::string& msisdn_dst, const std::string& text) {
    if (!context.getActive() || !context.getRegistered() || context.getTMSI() == 0) {
        std::cout << "UE is not registered. Use ACTIVE ON first." << std::endl;
        return false;
    }

    SmsOutData sms {};
    sms.smsId = nextSmsId.fetch_add(1);
    sms.msisdn_dst = msisdn_dst;
    sms.text = text;
    sms.status = "PENDING";
    context.addOutSms(sms);

    if (!sendRaw(makeSms(context.getTMSI(), msisdn_dst, sms.smsId, text))) {
        context.updateOutSmsStatus(sms.smsId, "SEND_ERROR");
        return false;
    }

    std::cout << "SMS send: id=" << sms.smsId << " dst=" << msisdn_dst << std::endl;
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
            PacketData packet {};
            size_t tookBytes {};

            if (!tryParseOne(rawBytes, packet, tookBytes)) {
                break;
            }

            handlePacket(packet);
            rawBytes.erase(0, tookBytes);
        }
    }

    running.store(false);
}

void Exchange::handlePacket(const PacketData& packet) {
    switch (packet.op) {
        case 'r': {
            std::lock_guard lock(authMutex);
            searchResults.push_back(SearchResultData{packet.stationId, packet.signalPower});
            break;
        }

        case 'a': {
            {
                std::lock_guard lock(authMutex);
                pendingTMSI = packet.tmsi;
            }

            condition.notify_all();
            std::cout << "Auth response: TMSI=" << packet.tmsi << std::endl;
            break;
        }

        case 'M': {
            if (packet.tmsi != context.getTMSI()) {
                break;
            }

            SmsInData sms {};
            sms.smsId = packet.smsId;
            sms.msisdn_src = packet.msisdn;
            sms.text = packet.text;
            context.addInSms(sms);

            std::cout << "\nSMS from " << sms.msisdn_src << ": " << sms.text << std::flush;

            sendRaw(makeDelivery(context.getTMSI(), sms.smsId, true));
            break;
        }

        case 'm': {
            context.updateOutSmsStatus(packet.smsId, packet.status ? "DELIVERED" : "FAILED");

            std::cout << "\nSMS id=" << packet.smsId
                      << " status=" << (packet.status ? "DELIVERED" : "FAILED/EXPIRED") << std::flush;
            break;
        }

        case 't': {
            std::cout << "SERVER: " << packet.text << std::endl;
            break;
        }

        case 'h': {
            context.setENodeb_id(packet.stationId);
        }

        default:
            break;
    }
}

void Exchange::printStatus() const {
    std::cout << "active=" << (context.getActive() ? "true" : "false")
              << " registered=" << (context.getRegistered() ? "true" : "false")
              << " pos=" << context.getCoordinate()
              << " station=" << context.getENodeb_id()
              << " tmsi=" << context.getTMSI() << std::endl;
}

void Exchange::printInbox() const {
    const auto inbox = context.getInSms();

    if (inbox.empty()) {
        std::cout << "Inbox is empty" << std::endl;
        return;
    }

    for (const auto& sms : inbox) {
        std::cout << "id=" << sms.smsId << " from=" << sms.msisdn_src
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
        std::cout << "id=" << sms.smsId << " to=" << sms.msisdn_dst
                  << " status=" << sms.status
                  << " text=" << sms.text << std::endl;
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
        std::cout << "  bs=" << station.bsId
                  << " power=" << std::fixed << station.power << std::endl;
    }
}