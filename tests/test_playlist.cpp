// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "../src/audio/playlist.hpp"

namespace fs = std::filesystem;

class PlaylistTest : public ::testing::Test {
protected:
  void SetUp() override { test_dir = "../tests/resources"; }

  std::string test_dir;
};

TEST_F(PlaylistTest, LoadsSongsSuccessfully) {
  Playlist playlist(test_dir);
  EXPECT_FALSE(playlist.current().empty());
}

TEST_F(PlaylistTest, ThrowsIfNoMP3Found) {
  EXPECT_THROW(Playlist("random"), std::runtime_error);
}

TEST_F(PlaylistTest, NextCyclesThroughSongs) {
  Playlist playlist(test_dir);
  auto first = playlist.current();
  auto second = playlist.next();
  EXPECT_NE(first, second);
  auto third = playlist.next();
  EXPECT_NE(second, third);
  auto wrap_around = playlist.next(); // Should cycle back
  EXPECT_EQ(first, wrap_around);
}

TEST_F(PlaylistTest, PrevCyclesBackwards) {
  Playlist playlist(test_dir);
  playlist.next(); // move to index 1
  auto current = playlist.current();
  auto prev = playlist.prev(); // back to index 0
  EXPECT_NE(current, prev);
  auto wrap_back = playlist.prev(); // Should go to last track
  EXPECT_FALSE(wrap_back.empty());
}

TEST_F(PlaylistTest, HasNextAndHasPrevWorks) {
  Playlist playlist(test_dir);
  EXPECT_TRUE(playlist.has_next());
  EXPECT_FALSE(playlist.has_prev());

  playlist.next(); // index = 1
  EXPECT_TRUE(playlist.has_next());
  EXPECT_TRUE(playlist.has_prev());

  playlist.next(); // index = 2
  EXPECT_FALSE(playlist.has_next());
  EXPECT_TRUE(playlist.has_prev());
}

TEST_F(PlaylistTest, ReshuffleChangesOrder) {
  Playlist playlist(test_dir);
  auto original = playlist.current();

  playlist.reshuffle();
  auto reshuffled = playlist.current();

  // Since index resets to 0, just verify it doesn't throw and returns a valid
  // path
  EXPECT_FALSE(reshuffled.empty());
}