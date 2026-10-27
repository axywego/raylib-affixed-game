cmake_minimum_required(VERSION 3.20)
project(affixed-game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib 5.5 QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(game
    src/main.cpp
    src/core/projection.cpp
    src/core/config/config.cpp
    src/core/input/input.cpp
    src/core/pathfinding/pathfinder.cpp
    src/game/player/player.cpp
    src/game/scene/scene.cpp
    src/game/spell_system/spell_system.cpp
    src/game/collision/collision.cpp
    src/game/world/world.cpp
    src/game/enemy/enemy.cpp
    src/game/enemy/crowd.cpp
)
target_include_directories(game PRIVATE src)


target_link_libraries(game PRIVATE raylib)

if (MSVC)
    target_compile_options(game PRIVATE /W4)
else()
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()

# add_custom_command(TARGET game POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:game>/assets
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:game>/assets
#     COMMENT "Copying assets"
# )
