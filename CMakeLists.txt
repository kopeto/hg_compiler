cmake_minimum_required(VERSION 3.16)

project(hg_compiler
    VERSION 0.1.0
    DESCRIPTION "HG Compiler"
    LANGUAGES C CXX
)

# Language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful build options
option(BUILD_TESTS "Build unit tests" OFF)
option(ENABLE_WARNINGS "Enable extra compiler warnings" ON)
option(ENABLE_SANITIZERS "Enable sanitizers in Debug" OFF)
option(ENABLE_TIMERS "Enable timers for performance measurement" OFF)
option(ENABLE_PROFILER "Enable function profiling" OFF)

# Source files (adjust to your actual project structure)
file(GLOB_RECURSE HG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

if(NOT HG_SOURCES)
    message(WARNING "No source files found in src/.")
endif()

add_executable(${PROJECT_NAME} ${HG_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Compiler-specific warning flags
if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# Sanitizers (GCC/Clang, Debug mode)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
endif()

# Expose timer instrumentation as a numeric compile definition (0/1).
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HG_ENABLE_TIMERS=$<IF:$<BOOL:${ENABLE_TIMERS}>,1,0>
        HG_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        HG_DEFAULT_DICTIONARY_PATH="${CMAKE_SOURCE_DIR}/assets/words/garbi.txt"
        HG_DEFAULT_GRID_PATH="${CMAKE_SOURCE_DIR}/assets/grids/basic_6x6.grid"
        HG_ENABLE_PROFILER=$<IF:$<BOOL:${ENABLE_PROFILER}>,1,0>
)

# Threads (link now, useful if needed later)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Tests
if(BUILD_TESTS)
    enable_testing()

    add_executable(test_backtrack_correctness
        tests/test_backtrack_correctness.cpp
        src/crossword.cpp
        src/dict.cpp
        src/grid.cpp
        src/grid_word.cpp
        src/pattern.cpp
        src/word.cpp
    )

    target_include_directories(test_backtrack_correctness PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    # Añadir los mismos defines que hg_compiler
    target_compile_definitions(test_backtrack_correctness PRIVATE
        HG_ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/"
        HG_DEFAULT_GRID_PATH="${CMAKE_SOURCE_DIR}/assets/grids/basic_6x6.grid"
        HG_DEFAULT_DICTIONARY_PATH="${CMAKE_SOURCE_DIR}/assets/words/garbi.txt"
        HG_ENABLE_TIMERS=$<IF:$<BOOL:${ENABLE_TIMERS}>,1,0>
        HG_ENABLE_PROFILER=$<IF:$<BOOL:${ENABLE_PROFILER}>,1,0>
    )

    target_compile_features(test_backtrack_correctness PRIVATE cxx_std_20)

    add_test(
        NAME test_backtrack_correctness
        COMMAND test_backtrack_correctness
    )
endif()

# Install binary target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)