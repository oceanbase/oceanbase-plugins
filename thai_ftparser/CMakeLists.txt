CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

# TODO set `PLUGIN_NAME` to yours
SET(PLUGIN_NAME thai_ftparser)

# TODO The list below are your implementention files
SET(SOURCES
    thai_ftparser_main.cpp
    thai_parser_core.cpp
    thai_jni_bridge.cpp)

# You also should set the information below
PROJECT(${PLUGIN_NAME}
        DESCRIPTION "OceanBase Thai ftparser plugin with JNI support"
        HOMEPAGE_URL "https://open.oceanbase.com/"
        LANGUAGES CXX C ASM) # remove CXX if this is a pure C project


# Don't touch me
FIND_PACKAGE(ObPlugin REQUIRED)

# Find JNI for Java integration (only JVM, not AWT)
FIND_PACKAGE(JNI REQUIRED COMPONENTS JVM)

# Macro OB_ADD_PLUGIN is defined in ObPluginConfig.cmake which provided by oceanabse-plugin-devel
OB_ADD_PLUGIN(${PLUGIN_NAME}
  ${SOURCES}
)

# Add JNI include directories and libraries
TARGET_INCLUDE_DIRECTORIES(${PLUGIN_NAME} PRIVATE ${JNI_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} PRIVATE ${JNI_LIBRARIES})

# Set C++ standard for modern features
SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# Ensure symbols are exported
SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES CXX_VISIBILITY_PRESET default)