//
// Created by fisp on 08.04.2026.
//

#include "UeContext.hpp"

#include <iostream>
#include <mutex>
#include "HandleMessage.hpp"
#include "./baseStation/BaseStation.hpp"

#include <sys/socket.h>
#include <unistd.h>

UeContext::UeContext(const int socket, std::vector<std::shared_ptr<BaseStation>> stations):
                        clientSocket(socket), stationsOnline(std::move(stations)) {};

UeContext::~UeContext() {
    stop();
}

void UeContext::start() {
    running.store(true);
    listenerThread = std::thread(&UeContext::readSocket, this);
    std::cout << "UeContext created." << std::endl;
}

void UeContext::stop() {
    running.store(false);
    {
        std::lock_guard lock(socketMutex);
        if (clientSocket != -1) {
            close(clientSocket);
            clientSocket = -1;
        }
    }

    if (listenerThread.joinable()) {
        listenerThread.join();
    }
}

void UeContext::readSocket() {
    std::string rawBytes;
    char buffer[4096];

    while (running.load()) {
        const ssize_t readBytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if (readBytes > 0) {
            rawBytes.append(buffer, readBytes);
            HandleMessage parseRawBytes {};

            if (parseRawBytes.parserRawBytes(rawBytes).isGood()) {
                sendToBasestation(parseRawBytes);
                rawBytes.clear();
            }
        } else if (readBytes == 0) {
            break;
        }
    }
}

void UeContext::sendToBasestation(const HandleMessage& dataStruct) {
    if (dataStruct.getOperation() == 'R') {
        for (auto& station : stationsOnline) {
            station->addDataInBuffer(dataStruct, shared_from_this());
            std::cout << 1 << std::endl;
        }
        return;
    }

    if (chooseStation) {
        chooseStation->addDataInBuffer(dataStruct, shared_from_this());
    }
}

void UeContext::sendToClient(const std::string& message) {
    std::lock_guard lock(socketMutex);
    if (clientSocket != -1) {
        send(clientSocket, message.c_str(), message.size(), 0);
    }
}

void UeContext::setUserData(const std::string& IMSI, const std::string& MSISDN, const std::string& IMEI) {
    if (!ueData.IMSI.empty()) {
        return;
    }

    ueData.IMSI = IMSI;
    ueData.MSISDN = MSISDN;
    ueData.IMEI = IMEI;
}

void UeContext::setTMSI(const uint64_t TMSI) {
    if (TMSI == 0) {
        ueData.TMSI = TMSI;
    }
}

std::string UeContext::getIMSI() const{
    return ueData.IMSI;
}

void UeContext::setBasestation(const std::shared_ptr<BaseStation>& station) {
    chooseStation = station;
}

std::string UeContext::getMSISDN() const {
    return ueData.MSISDN;
}
