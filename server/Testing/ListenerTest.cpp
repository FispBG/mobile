#include <gtest/gtest.h>
//
// Created by fisp on 15.04.2026.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "listener/Listener.hpp"

TEST(ListenerTest, CreateServerSocketWorks) {
    Listener listener;
    const uint16_t port = 8080;

    const ResultStatus result = listener.createServerSocket(port, 4);
    EXPECT_TRUE(result.isGood());

    listener.stopServer();
}

TEST(ListenerTest, RunServerAcceptClientAndStop) {
    Listener listener;
    const uint16_t port = 8080;

    ASSERT_TRUE(listener.createServerSocket(port, 4).isGood());
    listener.setStationsOnline({});
    listener.runServer();

    const int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(clientSocket, 0);

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    ASSERT_EQ(connect(clientSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);

    close(clientSocket);
    listener.stopServer();
}
