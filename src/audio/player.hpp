#pragma once
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT
// License. See the LICENSE file in the project root for full license
// information.

#include <SDL2/SDL.h>
#include <mpg123.h>

#include <array>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <thread>

#include "playlist.hpp"

/**
 * @class Player
 * @brief Handles MP3 playback using SDL2 and libmpg123 with playlist support.
 *
 * The Player class is responsible for decoding and playing MP3 files,
 * managing playback state (play, pause, stop), handling volume control,
 * seeking, and transitioning between songs in a playlist.
 *
 * It uses SDL2 for audio output and libmpg123 for MP3 decoding.
 * Playback runs in a dedicated thread with cooperative cancellation.
 *
 * @note Playback and audio device interactions are thread-safe.
 */
class Player {
  friend class PlayerTest; ///< Allows test fixture to access private members
  friend class CLITest; ///< Allows CLI test fixture to access private members

  static constexpr auto SDL_AUDIO_BUFFER_SIZE = 8192U; ///< Audio buffer size
  static constexpr auto VOLUME_FULL = 1.0F;            ///< Max volume
  static constexpr auto VOLUME_MUTE = 0.0F;            ///< Muted volume
  static constexpr auto DEFAULT_FADE_DURATION =
      300; ///< Fade duration in milliseconds

  /**
   * @enum State
   * @brief Represents internal playback states.
   */
  enum class State : uint8_t {
    STOPPED = 0,   ///< Playback stopped
    PAUSE = 1,     ///< Playback paused
    PLAY = 2,      ///< Playback ongoing
    SWITCH_OFF = 3 ///< Shutdown state
  };

public:
  /**
   * @brief Constructs and initializes the Player.
   * Initializes SDL2 and mpg123, and starts the background playback thread.
   * @throws std::runtime_error if SDL or mpg123 fails to initialize.
   */
  Player();

  /**
   * @brief Destructor.
   * Stops playback and releases audio resources.
   */
  ~Player();

  Player(Player &player) = delete;
  Player(Player &&player) = delete;

  auto operator=(Player &player) -> Player & = delete;
  auto operator=(Player &&player) -> Player && = delete;

  /**
   * @brief Sets the playlist for the player.
   * @param playlist A unique pointer to a Playlist object.
   */
  void set_playlist(std::unique_ptr<Playlist> playlist);

  /// Loads and prepares the current song in the playlist.
  void load_current();

  /// Moves to the next song and starts playback.
  void next_song();

  /// Moves to the previous song and starts playback.
  void prev_song();

  /**
   * @brief Loads a specific song for playback.
   * @param path Filesystem path to the MP3 file.
   * @throws std::runtime_error if the file cannot be opened.
   */
  void load_song(const std::string &path);

  /// Pauses playback with a fade-out effect.
  void pause();

  /// Resumes playback with a fade-in effect.
  void resume();

  /**
   * @brief Seeks forward or backward in the current song.
   * @param seconds The number of seconds to seek relative to the current
   * position.
   */
  void seek_relative(int seconds);

  /**
   * @brief Checks if the player is currently playing.
   * @return true if playing, false otherwise.
   */
  [[nodiscard]] auto is_playing() const noexcept -> bool;

  /**
   * @brief Gets the playback progress.
   * @return A pair {elapsed_seconds, total_seconds}.
   */
  [[nodiscard]] auto get_progress() const noexcept -> std::pair<int, int>;

  /**
   * @brief Gets the song title if available from ID3 metadata.
   * @return Reference to the current song title string.
   */
  [[nodiscard]] auto get_title() const noexcept -> const std::string &;

  /**
   * @brief Gets the song artist if available from ID3 metadata.
   * @return Reference to the current song artist string.
   */
  [[nodiscard]] auto get_artist() const noexcept -> const std::string &;

  /**
   * @brief Accesses the currently loaded playlist.
   * @return A reference to the unique_ptr holding the Playlist.
   */
  [[nodiscard]] auto get_playlist() -> std::unique_ptr<Playlist> &;

  /**
   * @brief Sets the playback volume.
   * @param vol A float between 0.0 (mute) and 1.0 (full volume).
   */
  void set_volume(float vol);

  /**
   * @brief Adjusts volume by a delta.
   * @param delta Positive or negative float to increment/decrement volume.
   */
  void adjust_volume(float delta);

private:
  /// Internal thread function for managing playback loop.
  void player_thread(const std::stop_token &token);

  /// Streams audio from the MP3 decoder to the audio buffer.
  void stream_audio();

  /// Determines if playback should continue.
  [[nodiscard]] auto should_continue() const -> bool;

  /// Waits until enough buffer space is available before queuing more audio.
  void wait_until_buffer_has_space(unsigned delay_ms, unsigned multiplier);

  /// Waits for the audio buffer to fully drain.
  void wait_for_buffer_to_drain();

  /// Updates the elapsed time counter based on playback.
  void update_elapsed_time();

  /**
   * @brief Applies volume gain to raw audio buffer.
   * @param buffer Span of 16-bit PCM samples.
   */
  void apply_volume(std::span<int16_t> buffer);

  /**
   * @brief Gets the current volume level.
   * @return Volume as float between 0.0 and 1.0.
   */
  [[nodiscard]] auto get_volume() const -> float;

  /**
   * @brief Gradually fades to a new volume.
   * @param target Final volume level.
   * @param duration_ms Total duration of the fade in milliseconds.
   * @return A std::future that completes when fade is done.
   */
  [[nodiscard]] auto fade_to(float target,
                             int duration_ms = DEFAULT_FADE_DURATION)
      -> std::future<void>;

  /// Pauses the SDL audio device (if open).
  void pause_audio_device();

  /// Resumes the SDL audio device (if open).
  void resume_audio_device();

  // Thread-safe variables
  std::mutex audio_mutex_;                      ///< Protects audio state
  std::atomic<float> volume_{VOLUME_FULL};      ///< Current volume
  std::atomic<float> last_volume_{VOLUME_FULL}; ///< Volume before pause
  std::atomic<int> elapsed_seconds_{0};         ///< Playback progress
  std::atomic<State> state_{State::STOPPED};    ///< Current player state

  // Audio
  std::optional<SDL_AudioDeviceID> audio_device_{0}; ///< SDL audio handle
  mpg123_handle *mpg_handler_{nullptr};              ///< MP3 decoder handle
  std::array<char, SDL_AUDIO_BUFFER_SIZE> buffer_;   ///< PCM output buffer
  int32_t sample_rate_{0};                           ///< MP3 sample rate

  // Metadata
  std::string title_;    ///< Current song title
  std::string artist_;   ///< Current song artist
  int total_seconds_{0}; ///< Song duration in seconds

  // Timing
  std::chrono::steady_clock::time_point
      start_time_; ///< Playback start timestamp
  std::chrono::milliseconds elapsed_duration_{
      0}; ///< Duration before last pause

  // Playlist and thread
  std::unique_ptr<Playlist> playlist_; ///< Current playlist
  std::jthread player_thread_;         ///< Background playback thread
};