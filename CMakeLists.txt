cmake_minimum_required(VERSION 3.12)
project(dotui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add warnings and optimizations
if(NOT MSVC)
    add_compile_options(-Wall -Wextra -O3)
endif()

# Find ncurses library
find_package(Curses REQUIRED)

# Set up include directory
include_directories(src)
include_directories(${CURSES_INCLUDE_DIRS})

# Define core library target
add_library(core STATIC
    src/core/task.cpp
    src/core/task.hpp
    src/core/list.hpp
    src/core/theme.hpp
)

# Define input library target
add_library(input STATIC
    src/input/keybinds.cpp
    src/input/keybinds.hpp
    src/input/keycodes.hpp
)
target_link_libraries(input PUBLIC core)

# Define persistence library target
add_library(persistence STATIC
    src/persistence/persistence.cpp
    src/persistence/persistence.hpp
)
target_link_libraries(persistence PUBLIC core input)

# Define ui library target
add_library(ui STATIC
    src/ui/tui.cpp
    src/ui/tui.hpp
)
target_link_libraries(ui PUBLIC core input ${CURSES_LIBRARIES})

# Define app library target
add_library(app STATIC
    src/app/application.cpp
    src/app/application.hpp
)
target_link_libraries(app PUBLIC core persistence input ui ${CURSES_LIBRARIES})

# Main executable target linking to app
add_executable(dotui src/main.cpp)
target_link_libraries(dotui PRIVATE app)
