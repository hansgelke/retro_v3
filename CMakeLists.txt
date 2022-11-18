cmake_minimum_required(VERSION 3.5)

project(retro_v3 LANGUAGES C)

aux_source_directory(. SRC_LIST)

include_directories("include")

add_executable(retro_v3 ${SRC_LIST})

install(TARGETS retro_v3)

find_package (Threads)
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
