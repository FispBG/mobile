//
// Created by fisp on 14.04.2026.
//

#include "smsc/SMSC.hpp"

#include <gtest/gtest.h>

TEST(SMSCTest, CreateContextTakeTextAndReadIt) {
  SMSC smsc{std::chrono::milliseconds(5000)};

  ASSERT_TRUE(smsc.createSmsContext(1001, 77, "79990000001", "79990000002"));
  ASSERT_TRUE(smsc.takeSmsText(77, "hello", 5));

  std::string text;
  ASSERT_TRUE(smsc.getSmsText(77, text));
  EXPECT_EQ(text, "hello");
}

TEST(SMSCTest, DuplicateSmsIdIsRejected) {
  SMSC smsc{std::chrono::milliseconds(5000)};

  ASSERT_TRUE(smsc.createSmsContext(1001, 77, "79990000001", "79990000002"));
  EXPECT_FALSE(smsc.createSmsContext(1002, 77, "79990000003", "79990000004"));
}

TEST(SMSCTest, GetSourceDataWorksAfterTakeText) {
  SMSC smsc{std::chrono::milliseconds(5000)};

  ASSERT_TRUE(smsc.createSmsContext(1001, 77, "79990000001", "79990000002"));
  ASSERT_TRUE(smsc.takeSmsText(77, "hello", 15));

  uint64_t tmsiSrc{};
  int32_t bsId{};

  ASSERT_TRUE(smsc.getSourceTmsi(77, tmsiSrc));
  ASSERT_TRUE(smsc.getSourceBsId(77, bsId));

  EXPECT_EQ(tmsiSrc, 1001);
  EXPECT_EQ(bsId, 15);
}

// TEST(SMSCTest, DeleteAndAckRemoveContext) {
//   SMSC smsc{std::chrono::milliseconds(5000)};
//
//   ASSERT_TRUE(smsc.createSmsContext(1001, 77, "79990000001", "79990000002"));
//   EXPECT_TRUE(smsc.hasSmsContext(77));
//
//   smsc.deleteSmsContext(77);
//   EXPECT_FALSE(smsc.hasSmsContext(77));
//
//   ASSERT_TRUE(smsc.createSmsContext(1001, 88, "79990000001", "79990000002"));
//   EXPECT_TRUE(smsc.hasSmsContext(88));
//
//   smsc.deleteSmsContext(88);
//   EXPECT_FALSE(smsc.(88));
// }
