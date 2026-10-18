// Settings file tests, matching the Go (go/frontend/settings_test.go) and Rust
// (rust/src/frontend/settings.rs) versions.

#include <string>
#include <gtest/gtest.h>
#include "frontend/settings.h"

TEST(SettingsTest, ParsesQuirksAndSpeed) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(parseRomSettings("# Some Game\n\n  quirks = vip,wrap \r\nspeed=200\n", settings, error)) << error;
  ASSERT_TRUE(settings.quirks.has_value());
  EXPECT_TRUE(settings.quirks->vblank && settings.quirks->wrap);
  EXPECT_EQ(settings.speed, 200);
}

TEST(SettingsTest, QuirksStartFromTheDefault) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(parseRomSettings("quirks = vf-reset\n", settings, error)) << error;
  EXPECT_TRUE(settings.quirks->shift && settings.quirks->loadStore && settings.quirks->wrap);
  EXPECT_TRUE(settings.quirks->vfReset);

  ASSERT_TRUE(parseRomSettings("quirks = none,jump\n", settings, error)) << error;
  EXPECT_FALSE(settings.quirks->shift || settings.quirks->wrap);
  EXPECT_TRUE(settings.quirks->jump);
}

TEST(SettingsTest, LeavesMissingSettingsUnset) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(parseRomSettings("speed = 7\n", settings, error)) << error;
  EXPECT_FALSE(settings.quirks.has_value());
  EXPECT_EQ(settings.speed, 7);
}

TEST(SettingsTest, RejectsInvalidSettings) {
  RomSettings settings;
  std::string error;
  EXPECT_FALSE(parseRomSettings("quirks = octo\nspeed = fast\n", settings, error));
  EXPECT_EQ(error, "line 2: speed must be a number from 1 to 10000");
  EXPECT_FALSE(parseRomSettings("quirks = bogus\n", settings, error));
  EXPECT_EQ(error, "line 1: unknown quirk \"bogus\"");
  EXPECT_FALSE(parseRomSettings("colour = green\n", settings, error));
  EXPECT_EQ(error, "line 1: unknown setting \"colour\", expected quirks or speed");
  EXPECT_FALSE(parseRomSettings("speed\n", settings, error));
  EXPECT_EQ(error, "line 1: expected \"name = value\"");
  EXPECT_FALSE(parseRomSettings("speed = 0\n", settings, error));
}

TEST(SettingsTest, PathReplacesTheExtension) {
  EXPECT_EQ(romSettingsPath("roms/br8kout.ch8"), "roms/br8kout.cfg");
  EXPECT_EQ(romSettingsPath("roms/pong"), "roms/pong.cfg");
  EXPECT_EQ(romSettingsPath("my.roms/pong"), "my.roms/pong.cfg");
}
