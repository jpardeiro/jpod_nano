// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include "cli.hpp"

#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

class TerminalRawMode {
public:
  TerminalRawMode() {
    tcgetattr(STDIN_FILENO, &orig_);
    termios raw = orig_;
    raw.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  }

  ~TerminalRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_); }

private:
  termios orig_;
};

CLI::CLI(Player &player) : player_(player) {}

CLI::~CLI() { shutdown(); }

void CLI::handle_sigint(int signal) {
  if (signal == SIGINT) {
    sigint_received_ = true;
  }
}

void CLI::start() {
  std::signal(SIGINT, CLI::handle_sigint);

  input_thread_ =
      std::jthread([this](const std::stop_token &token) { input_loop(token); });
  display_thread_ = std::jthread(
      [this](const std::stop_token &token) { display_loop(token); });

  while (running_ && !sigint_received_) {
    static constexpr auto SLEEP = 200U;
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
  }

  shutdown();
}

void CLI::shutdown() {
  if (!running_) {
    return;
  }
  running_ = false;
  player_.pause();
}

void CLI::input_loop(const std::stop_token &token) {
  TerminalRawMode raw;
  std::cout
      << "Controls: SPACE = Play/Pause | a = -5s | d = +5s | ← → = Seek | "
         "+ = Vol+ | - = Vol- | s = Shuffle | n/p = Next/Prev | q = Quit\n";

  while (!token.stop_requested() && running_ && !sigint_received_) {
    int chr = getchar();
    if (chr == '\x1b') {
      handle_escape_sequence(); // Handles arrow keys
    } else {
      handle_key(chr); // All other keys
    }
  }
}

void CLI::display_loop(const std::stop_token &token) {
  while (!token.stop_requested() && running_ && !sigint_received_) {
    if (player_.get_progress().second > 0) {
      static constexpr auto TIME_CONVERSIONS = 60;
      auto [elapsed, total] = player_.get_progress(); // You’ll implement this
      int elapsed_min = elapsed / TIME_CONVERSIONS;
      int elapsed_sec = elapsed % TIME_CONVERSIONS;
      int total_min = total / TIME_CONVERSIONS;
      int total_sec = total % TIME_CONVERSIONS;

      static constexpr auto WIDTH = 30;
      int filled = (elapsed * WIDTH) / std::max(1, total);
      std::string bar(filled, '#');
      bar.resize(WIDTH, '-');
      std::cout << "\r[" << bar << "] " << std::setw(2) << std::setfill('0')
                << elapsed_min << ":" << std::setw(2) << std::setfill('0')
                << elapsed_sec << " / " << std::setw(2) << std::setfill('0')
                << total_min << ":" << std::setw(2) << std::setfill('0')
                << total_sec << " | " << player_.get_title() << " - "
                << player_.get_artist() << std::flush;
    }

    static constexpr auto SLEEP_MS = 100U;
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  }
}

void CLI::handle_key(int chr) {
  static constexpr int SEEK_RELATIVE = 5;
  static constexpr float VOLUME_DELTA = 0.1F;

  switch (chr) {
  case ' ':
    player_.is_playing() ? player_.pause() : player_.resume();
    break;
  case 'a':
  case 'A':
    player_.seek_relative(-SEEK_RELATIVE);
    break;
  case 'd':
  case 'D':
    player_.seek_relative(SEEK_RELATIVE);
    break;
  case 'q':
  case 'Q':
    sigint_received_ = true;
    break;
  case 'p':
  case 'P':
    player_.prev_song();
    break;
  case 'n':
  case 'N':
    player_.next_song();
    break;
  case '+':
    player_.adjust_volume(VOLUME_DELTA);
    break;
  case '-':
    player_.adjust_volume(-VOLUME_DELTA);
    break;
  case 's':
  case 'S':
    if (auto &playlist = player_.get_playlist()) {
      playlist->reshuffle();
    }
    break;
  default:
    break;
  }
}

void CLI::handle_escape_sequence() {
  if (getchar() != '[') {
    return;
  }

  static constexpr int SEEK_RELATIVE = 5;
  switch (getchar()) {
  case 'C': // →
    player_.seek_relative(SEEK_RELATIVE);
    break;
  case 'D': // ←
    player_.seek_relative(-SEEK_RELATIVE);
    break;
  default:
    break;
  }
}