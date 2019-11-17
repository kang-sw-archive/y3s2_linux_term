cmake_minimum_required(VERSION 2.8)
project(termp)

# Build configs
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

# Configure subdirectories
add_subdirectory(third)

# Include dirs

## DRIVER CONFIGURATION ##
# Driver should be built in manual way

## APP CONFIGURATION ##
# -- Source code definitions
aux_source_directory(src-app SRC_APP)
aux_source_directory(src-app/core SRC_CORE)

link_libraries(m)
set(FBG_SRC_DIR "${CMAKE_SOURCE_DIR}/third/fbg/src")
set(FBG_SRC ${FBG_SRC_DIR}/fbgraphics.c ${FBG_SRC_DIR}/nanojpeg/nanojpeg.c ${FBG_SRC_DIR}/lodepng/lodepng.c)

# Executable
add_executable(app ${SRC_APP} ${SRC_CORE} ${FBG_SRC})

# Dependencies
# -- uEmbedded
add_dependencies(app uembedded_c)
target_link_libraries(app PUBLIC uembedded_c)
target_include_directories(app PUBLIC ${FBG_SRC_DIR})
# -- fbg