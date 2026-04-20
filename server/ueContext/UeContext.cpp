//
// Created by fisp on 08.04.2026.
//

#include "UeContext.hpp"
#include "./baseStation/BaseStation.hpp"
#include "HandleMessage.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <mutex>

UeContext::UeContext(const int socket,
                     std::vector<std::shared_ptr<BaseStation>> stations)
    : clientSocket(socket), stationsOnline(std::move(stations)){};

UeContext::~UeContext() { stop(); }

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
    if (listenerThread.get_id() == std::this_thread::get_id()) {
      listenerThread.detach();
    } else {
      listenerThread.join();
    }
  }
}

void UeContext::readSocket() {
  std::string rawBytes;
  char buffer[4096];

  while (running.load()) {
    const ssize_t readBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (readBytes <= 0) {
      break;
    }

    rawBytes.append(buffer, readBytes);

    while (true) {
      size_t readBytesCount{};
      HandleMessage packet{};

      if (!packet.parserRawBytes(rawBytes, readBytesCount)) {
        break;
      }

      sendToBasestation(packet);
      rawBytes.erase(0, readBytesCount);
    }
  }
  stop();
}

void UeContext::sendToBasestation(const HandleMessage& dataStruct) {
  const char operation = dataStruct.getOperation();
  if (operation == 'R') {
    for (const auto& station : stationsOnline) {
      station->addDataInBuffer(dataStruct, shared_from_this());
    }
    return;
  }

  if (operation == 'A') {
    const auto data = std::get<RegisterUeData>(dataStruct.getData());

    for (const auto& station : stationsOnline) {
      if (station && station->getId() == data.stationId) {
        chooseStation = station;
        station->addDataInBuffer(dataStruct, shared_from_this());
        return;
      }
    }
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

void UeContext::setUserData(const std::string& IMSI, const std::string& MSISDN,
                            const std::string& IMEI) {
  if (!ueData.imsi.empty()) {
    return;
  }

  ueData.imsi = IMSI;
  ueData.msisdn = MSISDN;
  ueData.imei = IMEI;
}

void UeContext::setTMSI(const uint64_t TMSI) { ueData.tmsi = TMSI; }

std::string UeContext::getIMSI() const { return ueData.imsi; }

void UeContext::setBasestation(const std::shared_ptr<BaseStation>& station) {
  chooseStation = station;
}

std::string UeContext::getMSISDN() const { return ueData.msisdn; }

bool UeContext::isRunning() const { return running.load(); }

void UeContext::bsRequestToDeleteInactive(
    const std::shared_ptr<UeContext>& user) const {
  for (const auto& station : stationsOnline) {
    if (station) {
      station->removeInactiveUser(user);
    }
  }
}

std::vector<std::shared_ptr<BaseStation>> UeContext::getStationsOnline() const {
  return stationsOnline;
}