cmake_minimum_required(VERSION 3.16)

project("web-agent-hosting" 
  LANGUAGES ASM_MASM C CXX
  VERSION 1.0.0
)

include(.tools/targets_setting.cmake)
include(.tools/typescript/index.cmake)
include(.tools/NodeJS.cmake)

# Set default project properties
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
set(PROJECT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/.output)
set(PROJECT_OUTPUT_BUILD_DIR ${PROJECT_OUTPUT_DIR}/build)
set(PACKAGES_OUTPUT_DIR ${PROJECT_OUTPUT_DIR}/packages)

add_subdirectory("./webagent_hosting")
add_subdirectory("./node_webagent_host")
add_subdirectory("./oswin_webagent_host")

