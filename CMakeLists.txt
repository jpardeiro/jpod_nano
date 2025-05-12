cmake_minimum_required(VERSION 3.14)
project(jpod_nano)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always select compiler based on platform
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Use Homebrew LLVM Clang
    set(HOMEBREW_LLVM_BIN "/opt/homebrew/opt/llvm/bin")
    set(CMAKE_C_COMPILER "${HOMEBREW_LLVM_BIN}/clang" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER "${HOMEBREW_LLVM_BIN}/clang++" CACHE STRING "" FORCE)
    message(STATUS "Using Homebrew Clang at ${HOMEBREW_LLVM_BIN}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "" FORCE)
    message(STATUS "Using system GCC")
endif()

option(CODE_COVERAGE "Enable code coverage reporting" OFF)

if(CODE_COVERAGE)
    message(STATUS "Code coverage enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(MPG123 REQUIRED libmpg123)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/**/*.hpp"
    "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_SOURCE_DIR}/tests/*.hpp"
)

find_program(CLANG_FORMAT_EXE NAMES "clang-format")

include_directories(${SDL2_INCLUDE_DIRS} ${MPG123_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS} ${MPG123_LIBRARY_DIRS})

add_library(${PROJECT_NAME}_audio src/audio/player.cpp src/audio/playlist.cpp)
target_compile_options(${PROJECT_NAME}_audio PRIVATE ${SDL2_CFLAGS_OTHER} ${MPG123_CFLAGS_OTHER})
target_link_libraries(${PROJECT_NAME}_audio ${SDL2_LIBRARIES} ${MPG123_LIBRARIES})

add_library(${PROJECT_NAME}_cli src/cli/cli.cpp)
target_link_libraries(${PROJECT_NAME}_cli ${PROJECT_NAME}_audio)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_cli)

if(CLANG_FORMAT_EXE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_CXX_SOURCE_FILES}
        COMMENT "Automatically formatting source files"
    )
endif()

enable_testing()

find_package(GTest REQUIRED)

# --- Add test files ---
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/*.cpp")

add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}_tests
    GTest::gtest_main
    ${PROJECT_NAME}_cli
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)

if(CODE_COVERAGE)
    find_program(GCOVR_PATH gcovr REQUIRED)

    add_custom_target(coverage
        COMMAND ${GCOVR_PATH}
            -r ${CMAKE_SOURCE_DIR}
            --gcov-executable gcov
            --exclude-directories tests
            --exclude-directories /usr
            --html --html-details -o build/coverage.html
            --xml-pretty -o coverage.xml
            --txt -o coverage.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report with gcovr"
    )
endif()