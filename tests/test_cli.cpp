// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "../src/cli/cli.hpp"

class CLITest : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir = "../tests/resources";
    auto playlist = std::make_unique<Playlist>(test_dir);
    player.set_playlist(std::move(playlist));
  }

  void handle_key(int chr) { cli.handle_key(chr); }

  [[nodiscard]] auto get_volume() const -> float { return player.get_volume(); }

  [[nodiscard]] static auto sigint_received() -> bool {
    return CLI::sigint_received_;
  }

  std::string test_dir;
  Player player;
  CLI cli{player};
};

TEST_F(CLITest, HandlesPlayPauseToggle) {
  handle_key(' '); // Should call resume()
  EXPECT_TRUE(player.is_playing());

  handle_key(' '); // Should call pause()
  EXPECT_FALSE(player.is_playing());
}

TEST_F(CLITest, HandlesVolumeAdjustment) {
  float before = get_volume();
  handle_key('-');
  EXPECT_LT(get_volume(), before);

  handle_key('+');
  EXPECT_FLOAT_EQ(get_volume(), before); // Should go back to original
}

TEST_F(CLITest, HandlesNextAndPrev) {
  std::string original = player.get_title();
  handle_key('n');
  EXPECT_NE(player.get_title(), original);

  handle_key('p');
  EXPECT_EQ(player.get_title(), original);
}

TEST_F(CLITest, HandlesShuffleWithPlaylist) {
  handle_key('s');
  SUCCEED(); // If no crash or throw, success
}

TEST_F(CLITest, HandlesQuitKey) {
  EXPECT_FALSE(sigint_received());
  handle_key('q');
  EXPECT_TRUE(sigint_received());
}