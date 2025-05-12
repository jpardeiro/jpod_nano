// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include "playlist.hpp"

#include <algorithm>
#include <filesystem>
#include <random>
#include <stdexcept>

namespace fs = std::filesystem;

Playlist::Playlist(const std::string &folder_path) {
  load_songs(folder_path);
  if (songs_.empty()) {
    throw std::runtime_error("No MP3 files found in folder: " + folder_path);
  }
}

void Playlist::load_songs(const std::string &folder_path) {
  for (const auto &entry : fs::directory_iterator(folder_path)) {
    if (entry.is_regular_file()) {
      const auto &path = entry.path();
      if (path.extension() == ".mp3") {
        shuffle_order_.emplace_back(songs_.size());
        songs_.emplace_back(path.string());
      }
    }
  }
  std::ranges::sort(songs_, std::less{}, std::identity{});
}

auto Playlist::current() const -> const std::string & {
  return songs_.at(shuffle_order_[index_]);
}

auto Playlist::next() -> const std::string & {
  index_ = (index_ + 1) % songs_.size();
  return current();
}

auto Playlist::prev() -> const std::string & {
  if (index_ == 0) {
    index_ = songs_.size();
  }
  index_ = (index_ - 1) % songs_.size();
  return current();
}

auto Playlist::has_next() const -> bool { return index_ + 1 < songs_.size(); }

auto Playlist::has_prev() const -> bool { return index_ > 0; }

void Playlist::reshuffle() {
  shuffle_order_.resize(songs_.size());
  std::iota(shuffle_order_.begin(), shuffle_order_.end(), 0);
  std::shuffle(shuffle_order_.begin(), shuffle_order_.end(),
               std::mt19937{std::random_device{}()});
  index_ = 0;
}
