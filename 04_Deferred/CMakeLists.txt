cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# See: https://gitlab.kitware.com/cmake/cmake/-/issues/22246
if(POLICY CMP0112)
    cmake_policy(SET CMP0112 NEW)
endif()

project(05_Deferred)

#############################
#
#  Cmake file for oglbase for linux
#   (... and possibly for MacOS / any other *nix based OSes as well, but this hasn't been tested yet)
#
# How to compile the project:
#  0) make sure you have OpenGL, SDL2, glm and Glew installed
#  1) create a build directory somewhere and cd into it
#      for example: `mkdir build && cd build`
#  2) run this cmake file and specify the location of the linux OGLPack
#      for example: `cmake -DOGLPack_DIR="../3rd_party" ../OGLBase`
#      Note: substitute the "OGLBase" part with a path to the directory containing this CMakeLists.txt file
#  3) run `make` in the build directory whenever you have made a change to the source code / assets
#  4) your executable will be placed in the build directory
#
# How to add source files:
#  Place them inside the source folder.
#
# How to add assets:
#  Simply place them in the "Assets/" folder. It and the "Shaders/" folders will be copied over to
#  the build directory with all of their contents.
#
#############################

set(OGLPack_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party" CACHE PATH "Path to the Linux OGLPack")

set(ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Includes)
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

# Set the source files
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS
    *.c *.cpp Includes/*.c Includes/*.cpp
)
file(GLOB HEADER_FILES CONFIGURE_DEPENDS
    *.h *.hpp *.inl Includes/*.h Includes/*.hpp Includes/*.inl
)

# -------------

# Create our executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# On some linux distros, the FindSDL2_image file is missing, so we ship our own.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake")

# Find the required libraries
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
add_subdirectory(${OGLPack_DIR} 3rd_party_build)

# Set the include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
)

# Link the necessary libraries
target_link_libraries(${PROJECT_NAME}
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    IMGUI
)

# Copy Assets and Shaders to the build directory
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSET_DIR}/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR}/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders/
)
add_dependencies(${PROJECT_NAME} copy_assets)