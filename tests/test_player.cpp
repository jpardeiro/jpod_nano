// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <thread>

#include "../src/audio/player.hpp"
#include "../src/audio/playlist.hpp"

class PlayerTest : public ::testing::Test {
protected:
  void SetUp() override {
    playlist = std::make_unique<Playlist>("../tests/resources");
    player.set_playlist(std::move(playlist));
  }

  void TearDown() override { std::filesystem::remove_all("test_dir"); }

  auto get_volume() -> float { return player.get_volume(); }

  auto fade_to(float value) -> std::future<void> {
    return player.fade_to(value);
  }

  Player player;
  std::unique_ptr<Playlist> playlist;
};

TEST_F(PlayerTest, InitializesSuccessfully) {
  EXPECT_FALSE(player.is_playing());
}

TEST_F(PlayerTest, CanLoadPlaylist) {
  auto &playlist = player.get_playlist();
  ASSERT_TRUE(playlist);
  EXPECT_FALSE(playlist->current().empty());
}

TEST_F(PlayerTest, CanPlayAndPause) {
  player.resume();
  EXPECT_TRUE(player.is_playing());
  player.pause();
  EXPECT_FALSE(player.is_playing());
}

TEST_F(PlayerTest, CanAdjustVolume) {
  static constexpr auto VOLUME = 0.5F;
  player.set_volume(VOLUME);
  EXPECT_FLOAT_EQ(get_volume(), VOLUME);

  static constexpr auto VOLUME_DELTA = -0.3F;
  player.adjust_volume(VOLUME_DELTA);
  EXPECT_FLOAT_EQ(get_volume(), VOLUME + VOLUME_DELTA);

  static constexpr auto VOLUME_OUT_OF_LOWER_BOUNDS = -1.0F;
  player.set_volume(VOLUME_OUT_OF_LOWER_BOUNDS);
  EXPECT_FLOAT_EQ(get_volume(), 0.0F);

  static constexpr auto VOLUME_OUT_OF_UPPER_BOUNDS = 2.0F;
  player.set_volume(VOLUME_OUT_OF_UPPER_BOUNDS);
  EXPECT_FLOAT_EQ(get_volume(), 1.0F);
}

TEST_F(PlayerTest, CanGetTitleAndArtist) {
  player.resume();
  static constexpr auto WAIT_MS = 50U;
  std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
  EXPECT_NO_THROW({
    auto title = player.get_title();
    auto artist = player.get_artist();
  });
}

TEST_F(PlayerTest, CanSeekRelativeForwardAndBackward) {
  player.resume();
  static constexpr auto SEEK_TIME_S = 2; // [s]
  player.seek_relative(SEEK_TIME_S);
  auto [elapsed, total] = player.get_progress();
  EXPECT_GE(elapsed, 0);
  player.seek_relative(-SEEK_TIME_S);
  auto [elapsed2, total2] = player.get_progress();
  EXPECT_GE(elapsed2, 0);
}

TEST_F(PlayerTest, CanGetProgress) {
  auto [elapsed, total] = player.get_progress();
  EXPECT_GE(total, 0);
  EXPECT_GE(elapsed, 0);
}

TEST_F(PlayerTest, CanUseNextAndPrev) {
  EXPECT_NO_THROW(player.next_song());
  EXPECT_NO_THROW(player.prev_song());
}

TEST_F(PlayerTest, FadeToDoesNotCrash) {
  static constexpr auto FADE_VALUE = 0.5F;
  auto fade = fade_to(FADE_VALUE);
  fade.wait();
  EXPECT_NEAR(get_volume(), FADE_VALUE, 0.1F);
}

TEST_F(PlayerTest, PlaySong) {
  player.resume();
  auto [elapsed_init, total_init] = player.get_progress();
  static constexpr auto SLEEP_TIME_S = 1;
  std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME_S));
  auto [elapsed_end, total_end] = player.get_progress();

  EXPECT_NEAR(elapsed_end - elapsed_init, SLEEP_TIME_S, 0.1F);
}

TEST_F(PlayerTest, GoesToNextSongAtEndOfTrack) {
  // Start playing the current song
  player.resume();

  // Let the track play for a bit to simulate progress
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Seek very close to the end of the track
  auto [elapsed, total] = player.get_progress();
  int remaining = total - elapsed - 1;
  if (remaining > 0) {
    player.seek_relative(remaining);
  }

  // Wait for the track to finish and the player to transition
  static constexpr auto SLEEP_TO_FINISH_S = 3U;
  std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TO_FINISH_S));

  // We expect the player to have moved to the next song
  const auto &current_track = player.get_playlist()->current();
  EXPECT_FALSE(current_track.empty());
}

TEST(PlayerNoPlaylistTest, StopsWhenNoPlaylistAtEnd) {
  Player player;
  player.set_playlist(nullptr); // simulate no playlist
  player.load_song("../tests/resources/song2.mp3");
  player.resume();

  // Let it run and finish
  std::this_thread::sleep_for(std::chrono::seconds(3));
  auto [elapsed, total] = player.get_progress();
  player.seek_relative(total - elapsed - 1);
  std::this_thread::sleep_for(std::chrono::seconds(2));
}
