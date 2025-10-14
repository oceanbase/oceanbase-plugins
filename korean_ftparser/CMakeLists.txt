CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

# Set plugin name
SET(PLUGIN_NAME korean_ftparser)

# Source files
SET(SOURCES
    korean_ftparser_main.cpp
    korean_jni_bridge.cpp
)

# Project configuration
PROJECT(${PLUGIN_NAME}
        DESCRIPTION "Korean ftparser plugin"
        HOMEPAGE_URL "https://open.oceanbase.com/"
        LANGUAGES CXX C ASM)

# Find required packages
FIND_PACKAGE(ObPlugin REQUIRED)
FIND_PACKAGE(JNI REQUIRED COMPONENTS JVM)

# Add common JNI library as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common build_common_jni_lib)

# Use OB_ADD_PLUGIN macro (provided by ObPlugin)
OB_ADD_PLUGIN(${PLUGIN_NAME}
    ${SOURCES}
)

# Add include directories for common JNI library
TARGET_INCLUDE_DIRECTORIES(${PLUGIN_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common
    ${JNI_INCLUDE_DIRS}
)

# Link libraries
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} PRIVATE 
    ${JNI_LIBRARIES}
)

# Add link directory for common JNI library
TARGET_LINK_DIRECTORIES(${PLUGIN_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common/build
)

# Link the common JNI library
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} PRIVATE oceanbase_jni_common)

# Set C++ standard and RPATH
SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES 
    CXX_STANDARD 11 
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET default
    # RPATH settings: use $ORIGIN to find libraries in the same directory as the plugin
    BUILD_RPATH "\$ORIGIN:${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common/build"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH FALSE
)

# Add custom command to ensure common library is built first
ADD_CUSTOM_TARGET(build_common_jni_lib_korean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common/build --parallel
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../common/liboceanbase_jni_common
    COMMENT "Building common JNI library for Korean parser"
)

# Make plugin depend on common library
ADD_DEPENDENCIES(${PLUGIN_NAME} build_common_jni_lib_korean)
