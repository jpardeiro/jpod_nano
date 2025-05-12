#pragma once
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <atomic>
#include <string>
#include <thread>

#include "../audio/player.hpp"

/**
 * @class CLI
 * @brief Command-line interface for controlling audio playback via keyboard
 * input.
 *
 * The CLI class handles terminal interaction with the user, capturing
 * keypresses and dispatching commands to control playback through the Player
 * instance. It supports play/pause, volume, seeking, and song navigation, as
 * well as displaying song progress and metadata.
 */
class CLI {
  friend class CLITest; ///< Allows test fixture to access private members.

public:
  /**
   * @brief Constructs the CLI with a reference to an existing Player.
   * @param player The Player instance to control.
   */
  explicit CLI(Player &player);

  /**
   * @brief Destructor ensures proper shutdown and cleanup.
   */
  ~CLI();

  /**
   * @brief Starts the CLI, launching input and display threads.
   */
  void start();

private:
  /**
   * @brief Signal handler for SIGINT (Ctrl+C).
   * @param signal Signal number received.
   */
  static void handle_sigint(int signal);

  /**
   * @brief Handles a single character input from the user.
   * @param chr The character code pressed.
   */
  void handle_key(int chr);

  /**
   * @brief Handles multi-character escape sequences (e.g., arrow keys).
   */
  void handle_escape_sequence();

  /**
   * @brief Runs the input loop for processing user commands.
   * @param token Stop token to cancel the loop.
   */
  void input_loop(const std::stop_token &token);

  /**
   * @brief Runs the display loop to show playback progress and song info.
   * @param token Stop token to cancel the loop.
   */
  void display_loop(const std::stop_token &token);

  /**
   * @brief Gracefully shuts down the CLI and associated threads.
   */
  void shutdown();

  Player &player_;                  ///< Reference to the Player instance.
  std::atomic<bool> running_{true}; ///< Indicates whether the CLI is active.
  static inline std::atomic<bool> sigint_received_{
      false}; ///< Tracks SIGINT receipt.

  std::jthread input_thread_;   ///< Thread for handling user input.
  std::jthread display_thread_; ///< Thread for displaying playback info.
};