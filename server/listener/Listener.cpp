//
// Created by fisp on 08.04.2026.
//

#include "Listener.hpp"
#include "../commonFiles/utilityFunc/UtilityFunc.hpp"
#include "../ueContext/UeContext.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <thread>

ResultStatus Listener::createServerSocket(const uint16_t port,
                                          const int32_t maxConnections) {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  if (serverSocket < 0) {
    return RES_ERROR("Fail create socket.");
  }

  sockaddr_in serverSettings{};

  serverSettings.sin_family = AF_INET;
  serverSettings.sin_addr.s_addr = INADDR_ANY;
  serverSettings.sin_port = htons(port);

  if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverSettings),
           sizeof(serverSettings)) < 0) {
    close(serverSocket);
    serverSocket = -1;
    return RES_ERROR("Fail bind server socket.");
  }

  if (listen(serverSocket, maxConnections) < 0) {
    close(serverSocket);
    serverSocket = -1;
    return RES_ERROR("Fail listen server socket.");
  }

  return RES_GOOD("");
}

void Listener::runServer() {
  running.store(true);
  thread = std::thread(&Listener::acceptLoop, this);
}

void Listener::removeInactiveUsers() {
  std::lock_guard lock(mutex);

  deleteOneFromUnordMap(
      activeUsers,
      [&](const auto& user) { return !user.get() || !user.get()->isRunning(); },
      [&](const auto& user) {
        return user.get()->bsRequestToDeleteInactive(user);
      });
}

void Listener::acceptLoop() {
  while (running.load()) {
    removeInactiveUsers();

    sockaddr_in clientSettings{};
    socklen_t clientSettingsLen = sizeof(clientSettings);

    const int clientSocket =
        accept(serverSocket, reinterpret_cast<sockaddr*>(&clientSettings),
               &clientSettingsLen);

    if (clientSocket < 0) {
      if (!running.load()) {
        break;
      }
      continue;
    }

    std::cout << "User connected." << std::endl;

    const auto contextUser =
        std::make_shared<UeContext>(clientSocket, stationsOnline);
    {
      std::lock_guard lock(mutex);
      activeUsers.push_back(contextUser);
    }
    contextUser->start();
  }
}

void Listener::stopServer() {
  running.store(false);

  if (serverSocket != -1) {
    shutdown(serverSocket, SHUT_RDWR);
    close(serverSocket);
    serverSocket = -1;
  }

  if (thread.joinable()) {
    thread.join();
  }
}

Listener::~Listener() { stopServer(); }

void Listener::setStationsOnline(
    std::vector<std::shared_ptr<BaseStation>> stations) {
  stationsOnline = std::move(stations);
}
