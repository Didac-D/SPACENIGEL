cmake_minimum_required(VERSION 3.10)

project(SpaceNigel)

set(CMAKE_CXX_STANDARD 17)

# Manually specify GLFW paths
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS "C:/msys64/mingw64/include")
find_library(GLFW_LIBRARY NAMES glfw3 PATHS "C:/msys64/mingw64/lib")

# Find Packages
find_package(assimp REQUIRED)
find_package(Freetype REQUIRED)

# GLAD setup
add_library(glad STATIC external/src/glad.c)
target_include_directories(glad PUBLIC external/include)

# Main executable
add_executable(SpaceNigel
    src/main.cpp
    src/Game.cpp

    src/Ability.cpp
    src/Camera.cpp
    src/Font.cpp
    src/Model.cpp
    src/Particles.cpp
    src/Player.cpp
    src/Projectile.cpp
    src/Ship.cpp
)

# Include directories
target_include_directories(SpaceNigel PRIVATE
    external/include
    ${GLFW_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIRS}
    ${glm_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(SpaceNigel
    ${GLFW_LIBRARY}  
    ${FREETYPE_LIBRARIES}  
    glad
    assimp
    opengl32
)

# Copy necessary directories
add_custom_command(
    TARGET SpaceNigel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
)

add_custom_command(
    TARGET SpaceNigel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/settings.cfg.in
            ${CMAKE_BINARY_DIR}/settings.cfg
)