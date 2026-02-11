cmake_minimum_required(VERSION 3.30)
project(paclook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static linking for portability (Linux only, not supported on macOS)
if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

    # Option for full static linking (including libc)
    option(STATIC_LIBC "Statically link libc (requires musl or static glibc)" OFF)
    if(STATIC_LIBC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
endif()

add_executable(paclook src/main.cpp)

target_sources(paclook
    PUBLIC FILE_SET CXX_MODULES FILES
        src/package.cppm
        src/provider.cppm
        src/util.cppm
        src/terminal.cppm
        src/app.cppm
        src/providers/pacman.cppm
        src/providers/paru.cppm
        src/providers/yay.cppm
        src/providers/apt.cppm
        src/providers/dnf.cppm
        src/providers/apk.cppm
        src/providers/zypper.cppm
        src/providers/xbps.cppm
        src/providers/nix.cppm
        src/providers/brew.cppm
)

install(TARGETS paclook RUNTIME DESTINATION bin)
