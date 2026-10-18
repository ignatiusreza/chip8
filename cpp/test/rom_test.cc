// ROM settings file tests, matching the Go (go/chip8/rom_test.go) and Rust
// (rust/src/chip8/rom.rs) versions.

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "chip8/rom.h"

TEST(RomTest, ParsesQuirksAndSpeed) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(RomSettings::parse("# Some Game\n\n  quirks = vip,wrap \r\nspeed=200\n", settings, error)) << error;
  ASSERT_TRUE(settings.quirks.has_value());
  EXPECT_TRUE(settings.quirks->vblank && settings.quirks->wrap);
  EXPECT_EQ(settings.speed, 200);
}

TEST(RomTest, QuirksStartFromTheDefault) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(RomSettings::parse("quirks = vf-reset\n", settings, error)) << error;
  EXPECT_TRUE(settings.quirks->shift && settings.quirks->loadStore && settings.quirks->wrap);
  EXPECT_TRUE(settings.quirks->jumpV0 && settings.quirks->vfReset);

  ASSERT_TRUE(RomSettings::parse("quirks = schip,shift\n", settings, error)) << error;
  EXPECT_FALSE(settings.quirks->loadStore || settings.quirks->wrap || settings.quirks->jumpV0);
  EXPECT_TRUE(settings.quirks->shift);
}

TEST(RomTest, LeavesMissingSettingsUnset) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(RomSettings::parse("speed = 7\n", settings, error)) << error;
  EXPECT_FALSE(settings.quirks.has_value());
  EXPECT_EQ(settings.speed, 7);
}

TEST(RomTest, RejectsInvalidSettings) {
  RomSettings settings;
  std::string error;
  EXPECT_FALSE(RomSettings::parse("quirks = octo\nspeed = fast\n", settings, error));
  EXPECT_EQ(error, "line 2: speed must be a number from 1 to 10000");
  EXPECT_FALSE(RomSettings::parse("quirks = bogus\n", settings, error));
  EXPECT_EQ(error, "line 1: unknown quirk \"bogus\"");
  EXPECT_FALSE(RomSettings::parse("colour = green\n", settings, error));
  EXPECT_EQ(error, "line 1: unknown setting \"colour\", expected quirks, speed, title, author, about or controls");
  EXPECT_FALSE(RomSettings::parse("speed\n", settings, error));
  EXPECT_EQ(error, "line 1: expected \"name = value\"");
  EXPECT_FALSE(RomSettings::parse("speed = 0\n", settings, error));
}

TEST(RomTest, ParsesHowToPlay) {
  RomSettings settings;
  std::string error;
  ASSERT_TRUE(RomSettings::parse(
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

TEST(RomTest, PathReplacesTheExtension) {
  EXPECT_EQ(RomSettings::path("roms/br8kout.ch8"), "roms/br8kout.cfg");
  EXPECT_EQ(RomSettings::path("roms/pong"), "roms/pong.cfg");
  EXPECT_EQ(RomSettings::path("my.roms/pong"), "my.roms/pong.cfg");
}
