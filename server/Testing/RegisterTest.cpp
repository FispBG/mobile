//
// Created by fisp on 14.04.2026.
//

#include "TestConfig.hpp"
#include "register/Register.hpp"

#include <gtest/gtest.h>
#include <filesystem>

TEST(RegistrationTest, AddUserAndCheckAuthInfo) {
  Registration registration(defaultConfig::dbPath);
  ASSERT_TRUE(registration.start());

  ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));

  EXPECT_TRUE(registration.requestAuthInfo("1", "00101"));
  EXPECT_FALSE(registration.requestAuthInfo("2", "00101"));
  EXPECT_FALSE(registration.requestAuthInfo("1", "unknown-imsi"));
}

TEST(RegistrationTest, UpdateLocationAndResolveDestination) {
  Registration registration(defaultConfig::dbPath);
  ASSERT_TRUE(registration.start());

  ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
  ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));

  uint64_t tmsiDst{};
  int32_t bsId{};

  ASSERT_TRUE(registration.resolveDestination("79990000001", tmsiDst, bsId));
  EXPECT_EQ(tmsiDst, 1001);
  EXPECT_EQ(bsId, 7);
}

TEST(RegistrationTest, ChangePathToUeUpdatesStationId) {
  Registration registration(defaultConfig::dbPath);
  ASSERT_TRUE(registration.start());

  ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
  ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));
  ASSERT_TRUE(registration.changePathToUe(1001, 9));

  uint64_t tmsiDst{};
  int32_t bsId{};

  ASSERT_TRUE(registration.resolveDestination("79990000001", tmsiDst, bsId));
  EXPECT_EQ(tmsiDst, 1001);
  EXPECT_EQ(bsId, 9);
}

TEST(RegistrationTest, GetMsisdnByTmsiReturnsFalseForUnknownTmsi) {
  Registration registration(defaultConfig::dbPath);
  ASSERT_TRUE(registration.start());

  std::string msisdn;
  EXPECT_FALSE(registration.getMsisdnByTmsi(9999, msisdn));

  ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
  ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));
  ASSERT_TRUE(registration.getMsisdnByTmsi(1001, msisdn));
  EXPECT_EQ(msisdn, "79990000001");
}
