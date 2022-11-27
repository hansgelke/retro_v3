cmake_minimum_required(VERSION 3.5)

project(retro_v3 LANGUAGES C)

aux_source_directory(. SRC_LIST)

include_directories("include")

add_executable(retro_v3 ${SRC_LIST})

install(TARGETS retro_v3)

find_package (Threads)
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

find_package (Threads)
include_directories(/srv/nfs/usr/include/gstreamer-1.0
                    /srv/nfs/usr/include/glib-2.0
                    /srv/nfs/usr/lib/glib-2.0/include)
target_link_libraries (${PROJECT_NAME}
${CMAKE_THREAD_LIBS_INIT} m
                    /srv/nfs/usr/lib/libasound.so
                    /srv/nfs/usr/lib/gstreamer-1.0/libgstalsa.so
                    /srv/nfs/usr/lib/libgstreamer-1.0.so
                    /srv/nfs/usr/lib/libgobject-2.0.so
                    /srv/nfs/usr/lib/libglib-2.0.so )
