# cmake/toolchain-mingw64.cmake
#
# Cross-compilation toolchain for Windows x86-64 using MinGW-w64 on Linux.
#
# Usage:
#   cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake \
#         -DQt6_DIR=/path/to/qt6-windows/lib/cmake/Qt6
#   cmake --build build-win -j$(nproc)

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers — prefer the POSIX thread model (needed for std::thread)
find_program(MINGW_CXX NAMES
    x86_64-w64-mingw32-g++-posix
    x86_64-w64-mingw32-g++
    REQUIRED
)
find_program(MINGW_CC NAMES
    x86_64-w64-mingw32-gcc-posix
    x86_64-w64-mingw32-gcc
    REQUIRED
)

set(CMAKE_C_COMPILER   "${MINGW_CC}")
set(CMAKE_CXX_COMPILER "${MINGW_CXX}")
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Where to look for target libraries/headers (not the host system)
set(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # programs run on host
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)    # libs from target sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
