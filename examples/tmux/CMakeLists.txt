cmake_minimum_required(VERSION 3.10)
project(mytmux)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VTERM REQUIRED vterm)
pkg_check_modules(NCURSES REQUIRED ncursesw)

# Check for util library for forkpty
find_library(UTIL_LIBRARY util)

add_executable(mytmux 
    src/main.cpp 
    src/Tab.cpp
)

target_include_directories(mytmux PRIVATE 
    ${VTERM_INCLUDE_DIRS} 
    ${NCURSES_INCLUDE_DIRS}
    src
)

target_link_libraries(mytmux PRIVATE 
    ${VTERM_LIBRARIES} 
    ${NCURSES_LIBRARIES}
)

if(UTIL_LIBRARY)
    target_link_libraries(mytmux PRIVATE ${UTIL_LIBRARY})
endif()
