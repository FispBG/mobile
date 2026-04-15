//
// Created by fisp on 14.04.2026.
//

#include <gtest/gtest.h>

#include <array>
#include <filesystem>

#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "HandleMessage.hpp"
#include "baseStation/BaseStation.hpp"
#include "mme/MME.hpp"
#include "register/Register.hpp"
#include "ueContext/UeContext.hpp"


struct RegistrationDb {
    RegistrationDb() {
        std::filesystem::remove("registration.db");
    }

    ~RegistrationDb() {
        std::filesystem::remove("registration.db");
    }
};

std::array<int, 2> createSocketPair() {
    std::array sockets {-1, -1};
    const int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data());
    EXPECT_EQ(result, 0);
    return sockets;
}

std::string readMessageFromSocket(const int socket) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socket, &readSet);

    timeval timeout {};
    timeout.tv_sec = 1;

    const int ready = select(socket + 1, &readSet, nullptr, nullptr, &timeout);
    EXPECT_GT(ready, 0);

    char buffer[4096] {};
    const ssize_t size = recv(socket, buffer, sizeof(buffer), 0);
    EXPECT_GT(size, 0);

    return std::string(buffer, static_cast<std::size_t>(size));
}

TEST(UeContextTest, SetUserDataOnlyOnce) {
    auto user = std::make_shared<UeContext>(-1, std::vector<std::shared_ptr<BaseStation>> {});

    user->setUserData("00101", "79990000001", "1");
    user->setUserData("00999", "79990000999", "2");

    EXPECT_EQ(user->getIMSI(), "00101");
    EXPECT_EQ(user->getMSISDN(), "79990000001");
}

TEST(UeContextTest, SendToClientWritesBytesToSocket) {
    const auto sockets = createSocketPair();
    auto user = std::make_shared<UeContext>(sockets[0], std::vector<std::shared_ptr<BaseStation>> {});

    user->sendToClient("hello");

    const std::string rawBytes = readMessageFromSocket(sockets[1]);
    EXPECT_EQ(rawBytes, "hello");

    close(sockets[1]);
}

TEST(UeContextTest, BsRequestToDeleteInactiveRemovesUserFromStation) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    auto mme = std::make_shared<MME>(1, registration);
    auto station = std::make_shared<BaseStation>(10, 0, 1, 100.0f, 1, 16, mme);

    auto user = std::make_shared<UeContext>(-1, std::vector {station});

    ASSERT_TRUE(station->acceptHandover(777, user, UserBuffer {}));
    EXPECT_FALSE(station->canAcceptHandover());

    user->stop();
    user->bsRequestToDeleteInactive(user);

    EXPECT_TRUE(station->canAcceptHandover());
}
