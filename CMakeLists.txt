cmake_minimum_required(VERSION 3.16)

project(HitzGurutzatuak
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

# Threads (link now, useful if needed later)
find_package(Threads REQUIRED)

# Suppress Qt's internal Vulkan search (not needed for this project)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

# Find and link Qt6
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Source files (adjust to your actual project structure)
file(GLOB_RECURSE HG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

if(NOT HG_SOURCES)
    message(WARNING "No source files found in src/.")
endif()

add_executable(${PROJECT_NAME} 
    ${HG_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
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
        HG_ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/"
        HG_DEFAULT_DICTIONARY_PATH="${CMAKE_SOURCE_DIR}/assets/words/garbi_r_gabe.txt"
        HG_DEFAULT_GRID_PATH="${CMAKE_SOURCE_DIR}/assets/grids/basic_6x6.grid"
        HG_ENABLE_PROFILER=$<IF:$<BOOL:${ENABLE_PROFILER}>,1,0>
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads
    Qt6::Widgets
)

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

    target_compile_definitions(test_backtrack_correctness PRIVATE QT_NO_KEYWORDS)

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

# Install assets
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/"
    DESTINATION assets
)

# On Windows run windeployqt during install so CPack picks up all Qt DLLs
if(WIN32)
    # Find windeployqt next to qmake / in the Qt bin dir
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${CMAKE_PREFIX_PATH}/bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"Running windeployqt...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                        --release
                        --no-translations
                        \"\${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}.exe\"
                RESULT_VARIABLE ret
            )
            if(NOT ret EQUAL 0)
                message(FATAL_ERROR \"windeployqt failed\")
            endif()
        ")
    else()
        message(WARNING "windeployqt not found – Qt DLLs won't be bundled in the installer")
    endif()
endif()

# ── CPack / Installer ─────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME              "HitzGurutzatuak")
set(CPACK_PACKAGE_VENDOR            "HG")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HG Compiler – Crossword solver")
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HitzGurutzatuak")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE.txt")

# NSIS (Windows) ──────────────────────────────────────────────────────────────
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_DISPLAY_NAME         "HitzGurutzatuak")
set(CPACK_NSIS_PACKAGE_NAME         "HitzGurutzatuak")
set(CPACK_NSIS_INSTALL_ROOT         "$PROGRAMFILES64")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_FINISHPAGE_RUN   "bin\\\\HitzGurutzatuak.exe")

# Shortcut in Start Menu
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\HitzGurutzatuak.lnk' '$INSTDIR\\\\bin\\\\HitzGurutzatuak.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\HitzGurutzatuak.lnk'"
)

include(CPack)