cmake_minimum_required(VERSION 3.5)

project(clip)
set(CMAKE_C_FLAGS "-Wall -std=gnu11")

include(FindPkgConfig)
pkg_check_modules(GTK3 gtk+-3.0>=3.2.4)
pkg_check_modules(GLIB glib-2.0>=2.30.3)
pkg_check_modules(X11 x11>=1.4.3)
pkg_check_modules(SQLITE3 sqlite3>=3.7.7)


include_directories(${GTK3_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS})
add_definitions(${GTK3_CFLAGS} ${GLIB_CFLAGS} ${X11_CFLAGS} ${SQLITE3_CFLAGS})

file(GLOB SOURCES "src/*.c")

add_executable(clip ${SOURCES})
target_link_libraries(clip ${GLIB_LIBRARIES} ${GTK3_LIBRARIES} ${X11_LIBRARIES} ${SQLITE3_LIBRARIES})
install(TARGETS clip DESTINATION bin)
