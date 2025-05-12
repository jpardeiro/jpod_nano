// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Jose Pardeiro
//
// This file is part of the jpod-nano project and is licensed under the MIT License.
// See the LICENSE file in the project root for full license information.

#include <SDL2/SDL.h>
#include <mpg123.h>

#include <chrono>
#include <iostream>
#include <thread>
#include "audio/player.hpp"
#include "audio/playlist.hpp"
#include "cli/cli.hpp"


static constexpr auto SDL_AUDIO_BUFFER_SIZE = 4096U;

auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <song.mp3>\n";
        return 1;
    }

    const char* filename = argv[1];


    try {
        auto playlist = std::make_unique<Playlist>(filename);
        Player player;
        player.set_playlist(std::move(playlist));

        CLI cli(player);
        cli.start();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}