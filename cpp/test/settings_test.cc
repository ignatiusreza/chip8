// Settings file tests, matching the Go (go/frontend/settings_test.go) and Rust
// (rust/src/frontend/settings.rs) versions.

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "core/settings.h"

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
  EXPECT_TRUE(settings.quirks->jumpV0 && settings.quirks->vfReset);

  ASSERT_TRUE(parseRomSettings("quirks = schip,shift\n", settings, error)) << error;
  EXPECT_FALSE(settings.quirks->loadStore || settings.quirks->wrap || settings.quirks->jumpV0);
  EXPECT_TRUE(settings.quirks->shift);
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
  EXPECT_EQ(error, "line 1: unknown setting \"colour\", expected quirks, speed, title, author, about or controls");
  EXPECT_FALSE(parseRomSettings("speed\n", settings, error));
  EXPECT_EQ(error, "line 1: expected \"name = value\"");
  EXPECT_FALSE(parseRomSettings("speed = 0\n", settings, error));
}

TEST(SettingsTest, ParsesHowToPlay) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(parseRomSettings(
      "title = Br8kout\nauthor = SharpenedSpoon\n"
      "about = Break the bricks.\nabout = Don't miss the ball.\n"
      "controls = A / D: move left / right\ncontrols = E: serve\n", settings, error)) << error;
  EXPECT_EQ(settings.title, "Br8kout");
  EXPECT_EQ(settings.author, "SharpenedSpoon");
  EXPECT_EQ(settings.about, (std::vector<std::string>{"Break the bricks.", "Don't miss the ball."}));
  EXPECT_EQ(settings.controls, (std::vector<std::string>{"A / D: move left / right", "E: serve"}));
  EXPECT_FALSE(settings.quirks.has_value());
  EXPECT_FALSE(settings.speed.has_value());
}

TEST(SettingsTest, PathReplacesTheExtension) {
  EXPECT_EQ(romSettingsPath("roms/br8kout.ch8"), "roms/br8kout.cfg");
  EXPECT_EQ(romSettingsPath("roms/pong"), "roms/pong.cfg");
  EXPECT_EQ(romSettingsPath("my.roms/pong"), "my.roms/pong.cfg");
}
