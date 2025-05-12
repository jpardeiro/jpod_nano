#pragma once
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <string>
#include <vector>

/**
 * @class Playlist
 * @brief Manages a list of MP3 file paths and provides shuffle, navigation, and
 * access utilities.
 *
 * The Playlist class is responsible for loading MP3 files from a specified
 * folder, maintaining a shuffle order, and allowing navigation through the list
 * of songs.
 */
class Playlist {
public:
  /**
   * @brief Constructs a Playlist from the MP3 files in the given folder.
   *
   * @param folder_path Path to the directory containing MP3 files.
   * @throws std::runtime_error if no MP3 files are found.
   */
  explicit Playlist(const std::string &folder_path);

  /**
   * @brief Gets the currently selected song.
   *
   * @return Reference to the full path of the current MP3 file.
   */
  [[nodiscard]] auto current() const -> const std::string &;

  /**
   * @brief Moves to the next song in the playlist.
   *
   * @return Reference to the full path of the next MP3 file.
   */
  auto next() -> const std::string &;

  /**
   * @brief Moves to the previous song in the playlist.
   *
   * @return Reference to the full path of the previous MP3 file.
   */
  auto prev() -> const std::string &;

  /**
   * @brief Checks if there is a next song available.
   *
   * @return true if a next song exists, false otherwise.
   */
  [[nodiscard]] auto has_next() const -> bool;

  /**
   * @brief Checks if there is a previous song available.
   *
   * @return true if a previous song exists, false otherwise.
   */
  [[nodiscard]] auto has_prev() const -> bool;

  /**
   * @brief Randomly reshuffles the order of the songs.
   * Resets the index to the beginning.
   */
  void reshuffle();

private:
  /**
   * @brief Loads MP3 file paths from the given directory into the playlist.
   *
   * @param folder_path Directory containing MP3 files.
   */
  void load_songs(const std::string &folder_path);

  std::vector<std::string> songs_;    ///< List of full paths to MP3 files
  std::vector<size_t> shuffle_order_; ///< Current shuffle order of song indices
  size_t index_ = 0;                  ///< Index into the shuffle_order_ vector
};