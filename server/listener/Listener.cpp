//
// Created by fisp on 08.04.2026.
//

#include "Listener.hpp"
#include "../ueContext/UeContex.hpp"
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>


ResultStatus Listener::createServerSocket(const uint16_t port, const size_t maxConnections) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        RES_ERROR("Fail create socket.");
    }

    sockaddr_in serverSettings{};

    serverSettings.sin_family = AF_INET;
    serverSettings.sin_addr.s_addr = INADDR_ANY;
    serverSettings.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverSettings), sizeof(serverSettings)) < 0) {
        RES_ERROR("Fail bind server socket.");
    }

    if (listen(serverSocket, maxConnections) < 0) {
        RES_ERROR("Fail listen server socket.");
    }

    return RES_GOOD;
}

void Listener::runServer() {
    running = true;
    thread = std::thread(&Listener::acceptLoop, this);
    thread.detach();
}

void Listener::acceptLoop() const {
    while (running) {
        sockaddr_in clientSettings{};
        socklen_t clientSettingsLen = sizeof(clientSettings);

        const int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientSettings), &clientSettingsLen);

        if (clientSocket < 0) {
            if (!running) {
                break;
            }
            continue;
        }

        auto contextUser = UeContext(clientSocket);
        std::thread(&UeContext::start, &contextUser).detach();
    }
}

void Listener::stopServer() {
    running = false;

    if (thread.joinable()) {
        thread.join();
    }
}
