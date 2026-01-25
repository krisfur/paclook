cmake_minimum_required(VERSION 3.16)
project(paclook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static linking for portability
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Option for full static linking (including libc)
option(STATIC_LIBC "Statically link libc (requires musl or static glibc)" OFF)
if(STATIC_LIBC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/terminal.cpp
    src/providers/paru.cpp
    src/providers/yay.cpp
    src/providers/pacman.cpp
)

add_executable(paclook ${SOURCES})

target_include_directories(paclook PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Installation
install(TARGETS paclook RUNTIME DESTINATION bin)
