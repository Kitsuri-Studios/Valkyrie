cmake_minimum_required(VERSION 3.20)
project(valkyrie CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview.git
    GIT_TAG master
)
FetchContent_MakeAvailable(webview)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UV REQUIRED libuv)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)

# quickjs doesn't have pkg-config usually
find_path(QUICKJS_INCLUDE_DIR quickjs.h
    PATHS /usr/include /usr/local/include /usr/include/quickjs /usr/local/include/quickjs
)
find_library(QUICKJS_LIBRARY
    NAMES quickjs libquickjs.a
    PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib/quickjs
)


if(NOT QUICKJS_INCLUDE_DIR OR NOT QUICKJS_LIBRARY)
    message(FATAL_ERROR "QuickJS not found. Install with: git clone https://github.com/bellard/quickjs && cd quickjs && make && sudo make install")
endif()

add_executable(valkyrie src/cli/main.cpp)

target_include_directories(valkyrie PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${webview_SOURCE_DIR}
    ${UV_INCLUDE_DIRS}
    ${QUICKJS_INCLUDE_DIR}
    ${GTK_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
)

target_link_libraries(valkyrie 
    ${UV_LIBRARIES}
    ${QUICKJS_LIBRARY}
    ${GTK_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    pthread
    dl
    m
)
