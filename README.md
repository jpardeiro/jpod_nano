# 🎵 jpod_nano

**jpod_nano** is a lightweight command-line MP3 player written in modern C++20. It uses SDL2 for audio playback and mpg123 for decoding. It features fade-in/out effects, playlist control, and a keyboard-driven interface.

## ✨ Features

- 🎶 MP3 decoding using `libmpg123`
- 🔊 Audio playback via `SDL2`
- 💻 Cross-platform (tested on macOS/Linux)
- ⌨️ Keyboard controls for play/pause, seek, volume, and navigation
- 🎚️ Fade-in and fade-out volume transitions
- 🔀 Playlist reshuffling
- ✅ Unit testing with GoogleTest
- 📊 Code coverage support with `gcovr`


## 📁 Folder Structure

```
jpod-nano/
├── CMakeLists.txt         # Top-level build config
├── README.md              # Project documentation
├── LICENSE                # MIT License
├── src/
│   ├── main.cpp           # Entry point
│   ├── cli/
│   │   └── cli.{hpp,cpp}  # Command-line interface implementation
│   └── audio/
│       ├── player.{hpp,cpp}   # Core audio playback logic
│       └── playlist.{hpp,cpp} # Playlist handling
├── tests/
│   └── test_player.cpp    # GoogleTest unit tests
└── build/                 # CMake build directory (ignored by Git)
```

## ⚙️ Requirements

- 🛠️ CMake >= 3.14
- 🚀 C++20-compatible compiler
- 🎧 SDL2
- 🧠 libmpg123
- 🧪 GoogleTest (optional)
- 📈 gcov and gcovr (optional)

### 🧰 On macOS (with Homebrew)

```bash
brew install cmake sdl2 mpg123 llvm gcovr
```

### 🧰 On Linux (Debian/Ubuntu-based)

```bash
sudo apt update
sudo apt install cmake g++ libsdl2-dev libmpg123-dev gcovr
```

> 💡 For code coverage support, ensure you use `g++` and pass `-DCODE_COVERAGE=ON` when configuring.

## 🏗️ Build Instructions

```bash
git clone https://github.com/jpardeiro/jpod_nano.git
cd jpod_nano
cmake -B build -DCMAKE_CXX_COMPILER=g++ -DCODE_COVERAGE=ON
cmake --build build
```

## ▶️ Usage

```bash
./build/jpod_nano path/to/mp3/folder
```

## 🎮 Controls

| Key       | Action              |
|-----------|---------------------|
| Space     | ⏯️  Play/Pause toggle   |
| a / A / ← | ⏪  Seek backward 5s    |
| d / D / → | ⏩  Seek forward 5s     |
| + / -     | 🔊 Volume up/down      |
| s         | 🔀 Shuffle playlist    |
| n / N     | ⏭️  Next song           |
| p / P     | ⏮️  Previous song       |
| q         | ❌ Quit the player     |

## 🧪 Running Tests

```bash
cd build
ctest --output-on-failure
```

## 📈 Code Coverage

If built with `CODE_COVERAGE=ON`:

```bash
gcovr -r .. --html --html-details -o build/coverage.html
xdg-open build/coverage.html  # Linux
```

## 🛠 Developer Tools

### 🧹 Format code

```bash
cmake --build build --target format
```

### 🔍 Static Analysis (if `clang-tidy` is available)

```bash
cmake --build build --target clang-tidy
```

## 📄 License

📝 MIT License. See the [LICENSE](LICENSE) file for details.

## 👤 Author

Jose Pardeiro