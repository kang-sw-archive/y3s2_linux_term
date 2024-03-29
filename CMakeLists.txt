cmake_minimum_required(VERSION 2.8)
project(termp)

# Build configs
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-pointer-to-int-cast -Wno-format"
)

# Configure subdirectories
add_subdirectory(third)

# Include dirs
include_directories("/usr/include/cairo")

## DRIVER CONFIGURATION ##
# Driver should be built in manual way

## APP CONFIGURATION ##
# -- Source code definitions
aux_source_directory(src SRC_APP)
aux_source_directory(src/core SRC_CORE)

# set(FBG_SRC_DIR "${CMAKE_SOURCE_DIR}/third/fbg/src")
# set(FBG_SRC ${FBG_SRC_DIR}/fbgraphics.c ${FBG_SRC_DIR}/nanojpeg/nanojpeg.c ${FBG_SRC_DIR}/lodepng/lodepng.c)

# General dependency
link_libraries(m)
link_libraries(cairo)
link_libraries(pthread)
link_libraries(asound)

# Executable
add_executable(app ${SRC_APP} ${SRC_CORE} ${FBG_SRC} ${SRC_ARCH})

# Dependencies
# -- uEmbedded
add_dependencies(app uembedded_c)
target_link_libraries(app PUBLIC uembedded_c)
target_include_directories(app PUBLIC ${FBG_SRC_DIR} third/uEmbedded/src)
# -- fbg
