cmake_minimum_required(VERSION 3.10)

project(my_ui C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LVGL REQUIRED lvgl)

add_executable(my-ui main.c)

target_include_directories(my-ui PRIVATE
    ${LVGL_INCLUDE_DIRS}
)

target_link_libraries(my-ui
    lvgl
    lvgl_thorvg
    m
)