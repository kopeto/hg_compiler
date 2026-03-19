cmake_minimum_required(VERSION 3.16)

project(eeh_query
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

# Qt6 installation path – adjust if your Qt is installed elsewhere
set(QT6_DIR "C:/Qt/6.10.2/msvc2022_64" CACHE PATH "Qt6 installation prefix")
list(APPEND CMAKE_PREFIX_PATH "${QT6_DIR}")

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

add_executable(eeh_query query.cpp)

target_link_libraries(eeh_query PRIVATE Qt6::Core Qt6::Sql)
