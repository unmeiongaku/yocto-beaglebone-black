cmake_minimum_required(VERSION 3.10)

project(appui C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LVGL REQUIRED lvgl)

add_executable(appui main.c)

target_include_directories(appui PRIVATE
    ${LVGL_INCLUDE_DIRS}
)

target_link_libraries(appui
    lvgl
    lvgl_thorvg
    m
)