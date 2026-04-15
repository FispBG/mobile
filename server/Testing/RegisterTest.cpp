//
// Created by fisp on 14.04.2026.
//

#include <gtest/gtest.h>

#include <filesystem>

#include "register/Register.hpp"


struct RegistrationDb {
    RegistrationDb() {
        std::filesystem::remove("registration.db");
    }

    ~RegistrationDb() {
        std::filesystem::remove("registration.db");
    }
};


TEST(RegistrationTest, AddUserAndCheckAuthInfo) {
    RegistrationDb guard;
    Registration registration;

    ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));

    EXPECT_TRUE(registration.requestAuthInfo("1", "00101"));
    EXPECT_FALSE(registration.requestAuthInfo("2", "00101"));
    EXPECT_FALSE(registration.requestAuthInfo("1", "unknown-imsi"));
}

TEST(RegistrationTest, UpdateLocationAndResolveDestination) {
    RegistrationDb guard;
    Registration registration;

    ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
    ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));

    uint64_t tmsiDst {};
    int32_t bsId {};

    ASSERT_TRUE(registration.resolveDestination("79990000001", tmsiDst, bsId));
    EXPECT_EQ(tmsiDst, 1001);
    EXPECT_EQ(bsId, 7);
}

TEST(RegistrationTest, ChangePathToUeUpdatesStationId) {
    RegistrationDb guard;
    Registration registration;

    ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
    ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));
    ASSERT_TRUE(registration.changePathToUe(1001, 9));

    uint64_t tmsiDst {};
    int32_t bsId {};

    ASSERT_TRUE(registration.resolveDestination("79990000001", tmsiDst, bsId));
    EXPECT_EQ(tmsiDst, 1001);
    EXPECT_EQ(bsId, 9);
}

TEST(RegistrationTest, GetMsisdnByTmsiReturnsFalseForUnknownTmsi) {
    RegistrationDb guard;
    Registration registration;

    std::string msisdn;
    EXPECT_FALSE(registration.getMsisdnByTmsi(9999, msisdn));

    ASSERT_TRUE(registration.addUsers("00101", "1", "79990000001"));
    ASSERT_TRUE(registration.updateLocation(1001, 7, "00101"));
    ASSERT_TRUE(registration.getMsisdnByTmsi(1001, msisdn));
    EXPECT_EQ(msisdn, "79990000001");
}
