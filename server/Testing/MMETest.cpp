//
// Created by fisp on 15.04.2026.
//

#include <gtest/gtest.h>

#include <filesystem>

#include "baseStation/BaseStation.hpp"
#include "mme/MME.hpp"
#include "register/Register.hpp"

struct RegistrationDb {
    RegistrationDb() {
        std::filesystem::remove("registration.db");
    }

    ~RegistrationDb() {
        std::filesystem::remove("registration.db");
    }
};

TEST(MMETest, GenerateTmsiReturnsZeroForUnknownUser) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    auto mme = std::make_shared<MME>(7, registration);

    const uint64_t tmsi = mme->generateTMSI("1", "unknown-imei", 10);
    EXPECT_EQ(tmsi, 0);
}

TEST(MMETest, GenerateTmsiUsesMmeIdInHighBits) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    ASSERT_TRUE(registration->addUsers("00101", "1", "79990000001"));

    auto mme = std::make_shared<MME>(7, registration);
    const uint64_t tmsi = mme->generateTMSI("00101", "1", 10);

    ASSERT_NE(tmsi, 0);
    EXPECT_EQ(tmsi >> 32, 7);
}

TEST(MMETest, ConfirmRegisterUpdatesRouteInRegistration) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    ASSERT_TRUE(registration->addUsers("00101", "1", "79990000001"));

    auto mme = std::make_shared<MME>(7, registration);
    const uint64_t tmsi = mme->generateTMSI("00101", "1", 10);

    ASSERT_NE(tmsi, 0);
    ASSERT_TRUE(mme->confirmRegister("00101", "79990000001", tmsi, 10));

    uint64_t resolvedTmsi {};
    int32_t bsId {};

    ASSERT_TRUE(registration->resolveDestination("79990000001", resolvedTmsi, bsId));
    EXPECT_EQ(resolvedTmsi, tmsi);
    EXPECT_EQ(bsId, 10);
}

TEST(MMETest, ResolveSmsRouteReturnsRegisteredStation) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    ASSERT_TRUE(registration->addUsers("00101", "1", "79990000001"));

    auto mme = std::make_shared<MME>(7, registration);
    auto station = std::make_shared<BaseStation>(10, 0, 7, 100.0f, 4, 16, mme);
    mme->registerStation(10, station);

    const uint64_t tmsi = mme->generateTMSI("00101", "1", 10);
    ASSERT_NE(tmsi, 0);
    ASSERT_TRUE(mme->confirmRegister("00101", "79990000001", tmsi, 10));

    uint64_t resolvedTmsi {};
    std::shared_ptr<BaseStation> resolvedStation;

    ASSERT_TRUE(mme->resolveSmsRoute("79990000001", resolvedTmsi, resolvedStation));
    EXPECT_EQ(resolvedTmsi, tmsi);
    ASSERT_TRUE(resolvedStation);
    EXPECT_EQ(resolvedStation->getId(), 10);
}

TEST(MMETest, ChangePathToUeUpdatesDestinationBaseStation) {
    RegistrationDb guard;
    auto registration = std::make_shared<Registration>();
    ASSERT_TRUE(registration->addUsers("00101", "2", "79990000001"));

    auto mme = std::make_shared<MME>(7, registration);
    const uint64_t tmsi = mme->generateTMSI("00101", "2", 10);

    ASSERT_NE(tmsi, 0);
    ASSERT_TRUE(mme->confirmRegister("00101", "79990000001", tmsi, 10));
    ASSERT_TRUE(mme->changePathToUe(tmsi, 22));

    uint64_t resolvedTmsi {};
    int32_t bsId {};

    ASSERT_TRUE(registration->resolveDestination("79990000001", resolvedTmsi, bsId));
    EXPECT_EQ(resolvedTmsi, tmsi);
    EXPECT_EQ(bsId, 22);
}
