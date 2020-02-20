cmake_minimum_required(VERSION 3.16)

project("web-agent-hosting" 
  LANGUAGES ASM_MASM C CXX
  VERSION 1.0.0
)

include(.tools/index.cmake)

# Set default project properties
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
set(PROJECT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/.output)
set(PROJECT_OUTPUT_BUILD_DIR ${PROJECT_OUTPUT_DIR}/build)
set(PACKAGES_OUTPUT_DIR ${PROJECT_OUTPUT_DIR}/packages)

add_subdirectory("./Hosting/webagent_hosting")
add_subdirectory("./Hosting/node_webagent_hosting")
add_subdirectory("./Hosting/oswin_webagent_hosting")

