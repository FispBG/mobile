//
// Created by fisp on 15.04.2026.
//

#include "./TestConfig.hpp"
#include "baseStation/BaseStation.hpp"
#include "mme/MME.hpp"
#include "register/Register.hpp"
#include "ueContext/HandleMessage.hpp"
#include "ueContext/UeContext.hpp"

#include <gtest/gtest.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <filesystem>

std::array<int, 2> createSockPair() {
  std::array sockets{-1, -1};
  const int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data());
  EXPECT_EQ(result, 0);
  return sockets;
}

std::string readMessageFromSock(const int socket) {
  fd_set readSet;
  FD_ZERO(&readSet);
  FD_SET(socket, &readSet);

  timeval timeout{};
  timeout.tv_sec = 1;

  const int ready = select(socket + 1, &readSet, nullptr, nullptr, &timeout);
  EXPECT_GT(ready, 0);

  char buffer[4096]{};
  const ssize_t size = recv(socket, buffer, sizeof(buffer), 0);
  EXPECT_GT(size, 0);

  return std::string(buffer, static_cast<std::size_t>(size));
}

TEST(BaseStationTest, CalculateSignalPowerWorksForNearAndFarPoints) {
  auto registration = std::make_shared<Registration>(defaultConfig::dbPath);
  auto mme = std::make_shared<MME>(1, registration);
  auto station = std::make_shared<BaseStation>(10, 100, 1, 50.0f, 4, 16, mme);

  EXPECT_DOUBLE_EQ(station->calculateSignalPower(100), 1.0);
  EXPECT_DOUBLE_EQ(station->calculateSignalPower(150), 0.0);
  EXPECT_LT(station->calculateSignalPower(200), 0.0);
}

TEST(BaseStationTest, AcceptHandoverReserveBufferAndSendSmsToUser) {
  auto registration = std::make_shared<Registration>(defaultConfig::dbPath);
  auto mme = std::make_shared<MME>(1, registration);
  auto station = std::make_shared<BaseStation>(10, 0, 1, 100.0f, 1, 16, mme);

  const auto sockets = createSockPair();
  auto user = std::make_shared<UeContext>(sockets[0], std::vector{station});

  ASSERT_TRUE(station->acceptHandover(1001, user, UserBuffer{}));
  EXPECT_FALSE(station->canAcceptHandover());

  ASSERT_TRUE(station->MMEReserveBuffer(1001, 77));
  ASSERT_TRUE(station->MMESendTextSms(1001, 77, "79990000001", "hello"));

  const std::string rawBytes = readMessageFromSock(sockets[1]);
  HandleMessage message;

  size_t read;
  ASSERT_TRUE(message.parserRawBytes(rawBytes, read));
  EXPECT_EQ(message.getOperation(), 'M');

  close(sockets[1]);
}
