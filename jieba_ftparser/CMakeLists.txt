CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

SET(PLUGIN_NAME jieba_ftparser)

SET(SOURCES
    ob_jieba_ftparser.cpp)

PROJECT(${PLUGIN_NAME}
        DESCRIPTION "OceanBase jieba ftparser plugin demo"
        HOMEPAGE_URL "https://open.oceanbase.com/"
        LANGUAGES CXX C ASM)

SET(DICT_RESOURCES "")
SET(_DICT_RESOURCE_ITEMS)
FILE(GLOB _DICT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cppjieba/dict/*.utf8")
FOREACH(DICT_FILE ${_DICT_FILES})
  get_filename_component(DICT_FILE_NAME ${DICT_FILE} NAME)
  FILE(READ ${DICT_FILE} DICT_CONTENT)
  LIST(APPEND _DICT_RESOURCE_ITEMS "{\"${DICT_FILE_NAME}\", R\"(${DICT_CONTENT})\"}\n")
ENDFOREACH()
STRING(JOIN ", " DICT_RESOURCES ${_DICT_RESOURCE_ITEMS})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/resources.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp)
LIST(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp)

# Don't touch me
FIND_PACKAGE(ObPlugin REQUIRED)

# Override the settings in ObPluginConfig
# The default cxx standard is 11
SET(CMAKE_CXX_STANDARD 17)

# Macro OB_ADD_PLUGIN is defined in ObPluginConfig.cmake which provided by oceanabse-plugin-devel
OB_ADD_PLUGIN(${PLUGIN_NAME}
  ${SOURCES}
)
MESSAGE (STATUS "PLUGIN_NAME is " ${PLUGIN_NAME})
TARGET_INCLUDE_DIRECTORIES (${PLUGIN_NAME} PRIVATE
                            ${CMAKE_CURRENT_SOURCE_DIR}/cppjieba/include
			                      ${CMAKE_CURRENT_SOURCE_DIR}/limonp/include)
# TARGET_LINK_LIBRARIES (${PLUGIN_NAME} library1 library2)
# TARGET_XX (${PLUGIN_NAME} xxx)
