// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include "player.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <thread>

Player::Player() {
  // Init libraries
  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    throw std::runtime_error("SDL_Init failed: " + std::string(SDL_GetError()));
  }

  if (mpg123_init() != MPG123_OK) {
    throw std::runtime_error("mpg123_init failed");
  }

  mpg_handler_ = mpg123_new(nullptr, nullptr);
  if (mpg_handler_ == nullptr) {
    throw std::runtime_error("mpg123_new failed");
  }

  player_thread_ = std::jthread(
      [this](const std::stop_token &token) { player_thread(token); });
}

Player::~Player() {
  // Stop the player
  state_.store(State::SWITCH_OFF);

  // Clean up
  {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (audio_device_.has_value()) {
      SDL_CloseAudioDevice(audio_device_.value());
      audio_device_.reset();
    }
  }
  mpg123_close(mpg_handler_);
  mpg123_delete(mpg_handler_);
  mpg123_exit();
  SDL_Quit();
}

void Player::set_playlist(std::unique_ptr<Playlist> playlist) {
  playlist_ = std::move(playlist);
  load_current();
}

void Player::load_current() {
  if (!playlist_) {
    return;
  }
  load_song(playlist_->current());
}

void Player::next_song() {
  if (playlist_) {
    pause();
    load_song(playlist_->next());
    resume();
  }
}

void Player::prev_song() {
  if (playlist_) {
    pause();
    load_song(playlist_->prev());
    resume();
  }
}

void Player::load_song(const std::string &path) {
  // Stop song
  state_.store(State::STOPPED);
  {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    pause_audio_device();
  }

  if (mpg123_open(mpg_handler_, path.c_str()) != MPG123_OK) {
    throw std::runtime_error("Failed to open " + path);
  }

  long rate = 0;
  int channels = 0;
  int encoding = 0;
  mpg123_getformat(mpg_handler_, &rate, &channels, &encoding);
  sample_rate_ = static_cast<int64_t>(rate);

  off_t total_samples = mpg123_length(mpg_handler_);
  if (total_samples != MPG123_ERR) {
    total_seconds_ = static_cast<int>(total_samples / rate);
  } else {
    total_seconds_ = 0;
  }
  elapsed_seconds_ = 0;
  elapsed_duration_ = std::chrono::seconds(0);

  mpg123_id3v1 *data1{nullptr};
  mpg123_id3v2 *data2{nullptr};
  auto meta = mpg123_meta_check(mpg_handler_);

  if ((meta & MPG123_ID3) != 0) {
    mpg123_id3(mpg_handler_, &data1, &data2);
    if (data2 != nullptr) {
      title_ = (data2->title != nullptr) ? data2->title->p : "";
      artist_ = (data2->artist != nullptr) ? data2->artist->p : "";
    } else if (data1 != nullptr) {
      title_ = data1->title;
      artist_ = data1->artist;
    }
  }

  SDL_AudioSpec want{};
  SDL_AudioSpec have{};
  SDL_zero(want);
  want.freq = rate;
  want.format = AUDIO_S16SYS;
  want.channels = channels;
  want.samples = SDL_AUDIO_BUFFER_SIZE;

  std::lock_guard<std::mutex> lock(audio_mutex_);
  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (audio_device_ == 0) {
    throw std::runtime_error("SDL_OpenAudioDevice error: " +
                             std::string(SDL_GetError()));
  }

  pause_audio_device();
}

void Player::pause() {
  if (state_.load() == State::SWITCH_OFF) {
    return;
  }
  auto now = std::chrono::steady_clock::now();
  elapsed_duration_ +=
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
  if (state_.load() != State::PAUSE) {
    last_volume_.store(get_volume());
  }
  auto fade_future = fade_to(VOLUME_MUTE);
  fade_future.wait();
  pause_audio_device();
  state_.store(State::PAUSE);
}

void Player::resume() {
  if (state_.load() == State::SWITCH_OFF) {
    return;
  }
  start_time_ = std::chrono::steady_clock::now();
  resume_audio_device();
  auto fade_future = fade_to(last_volume_.load());
  fade_future.wait();
  state_.store(State::PLAY);
}

void Player::player_thread(const std::stop_token &token) {
  while (!token.stop_requested() && should_continue()) {
    static constexpr auto SLEEP = 5U;
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    if (state_.load() != State::PLAY) {
      continue;
    }

    resume_audio_device();

    stream_audio();
    wait_for_buffer_to_drain();

    if (state_.load() == State::PLAY) {
      mpg123_close(mpg_handler_);
      if (playlist_) {
        next_song();
      } else {
        state_.store(State::STOPPED);
      }
    }
  }
}

auto Player::should_continue() const -> bool {
  return state_.load() != State::SWITCH_OFF;
}

void Player::stream_audio() {
  static constexpr auto DELAY_MS = 10U;
  static constexpr auto BUFFER_MULTIPLIER = 32U;
  size_t completed_bytes = 0;

  while ((state_.load() == State::PLAY) &&
         (mpg123_read(mpg_handler_, buffer_.data(), buffer_.size(),
                      &completed_bytes)) == MPG123_OK) {
    update_elapsed_time();
    wait_until_buffer_has_space(DELAY_MS, BUFFER_MULTIPLIER);
    apply_volume(std::span{reinterpret_cast<int16_t *>(buffer_.data()),
                           completed_bytes / 2});
    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (audio_device_.has_value()) {
      SDL_QueueAudio(audio_device_.value(), buffer_.data(), completed_bytes);
    }
  }
}

void Player::wait_until_buffer_has_space(unsigned delay_ms,
                                         unsigned multiplier) {
  while (state_.load() == State::PLAY) {
    bool buffer_ready = false;
    {
      std::lock_guard<std::mutex> lock(audio_mutex_);
      if (audio_device_.has_value()) {
        buffer_ready = SDL_GetQueuedAudioSize(audio_device_.value()) <=
                       SDL_AUDIO_BUFFER_SIZE * multiplier;
      }
    }
    if (buffer_ready) {
      break;
    }
    SDL_Delay(delay_ms);
  }
}

void Player::wait_for_buffer_to_drain() {
  static constexpr auto DELAY_MS = 50U;
  while (state_.load() == State::PLAY) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (!audio_device_.has_value()) {
      break;
    }
    if (SDL_GetQueuedAudioSize(audio_device_.value()) <= 0) {
      break;
    }
    SDL_Delay(DELAY_MS);
  }
}

void Player::update_elapsed_time() {
  auto now = std::chrono::steady_clock::now();
  elapsed_seconds_ =
      static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                           elapsed_duration_ + (now - start_time_))
                           .count());
}

auto Player::is_playing() const noexcept -> bool {
  return state_.load() == State::PLAY;
}

auto Player::get_progress() const noexcept -> std::pair<int, int> {
  return {elapsed_seconds_.load(), total_seconds_};
}

auto Player::get_title() const noexcept -> const std::string & {
  return title_;
}
auto Player::get_artist() const noexcept -> const std::string & {
  return artist_;
}

void Player::seek_relative(int delta_seconds) {
  std::lock_guard<std::mutex> lock(audio_mutex_);

  if (state_.load() == State::SWITCH_OFF || !audio_device_.has_value()) {
    return;
  }

  // Compute new position in seconds
  const int current = elapsed_seconds_.load();
  const int target = std::clamp(current + delta_seconds, 0, total_seconds_);

  // Seek to the new position
  long rate = 0;
  int channels = 0;
  int encoding = 0;
  mpg123_getformat(mpg_handler_, &rate, &channels, &encoding);
  const auto sample_offset = static_cast<off_t>(target * rate);
  if (mpg123_seek(mpg_handler_, sample_offset, SEEK_SET) == MPG123_ERR) {
    std::cerr << "[WARN] Seek failed\n";
    return;
  }

  // Reset buffer and timing
  if (audio_device_.has_value()) {
    SDL_ClearQueuedAudio(audio_device_.value());
  }
  elapsed_duration_ = std::chrono::seconds(target);
  start_time_ = std::chrono::steady_clock::now();
  resume();
}

void Player::set_volume(float vol) {
  volume_.store(std::clamp(vol, VOLUME_MUTE, VOLUME_FULL));
}

void Player::adjust_volume(float delta) { set_volume(get_volume() + delta); }

auto Player::get_volume() const -> float { return volume_.load(); }

auto Player::get_playlist() -> std::unique_ptr<Playlist> & { return playlist_; }

void Player::apply_volume(std::span<int16_t> buffer) {
  const auto volume = get_volume();
  std::ranges::for_each(buffer, [volume](int16_t &data) {
    data = static_cast<int16_t>(data * volume);
  });
}

auto Player::fade_to(float target, int duration_ms) -> std::future<void> {
  return std::async(std::launch::async, [this, target, duration_ms] {
    static constexpr auto N_STEPS = 10;
    auto step = (target - get_volume()) / N_STEPS;
    for (int i = 0; i < N_STEPS; ++i) {
      set_volume(get_volume() + step);
      std::this_thread::sleep_for(
          std::chrono::milliseconds(duration_ms / N_STEPS));
    }
    set_volume(target);
  });
}

void Player::pause_audio_device() {
  if (audio_device_.has_value()) {
    SDL_PauseAudioDevice(audio_device_.value(), 1);
  }
}

void Player::resume_audio_device() {
  if (audio_device_.has_value()) {
    SDL_PauseAudioDevice(audio_device_.value(), 0);
  }
}
