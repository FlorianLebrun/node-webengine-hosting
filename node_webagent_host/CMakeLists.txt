set(target "node_webagent_host")

function(append_node_webagent_host_ABI_library LIST target)
  cmake_parse_arguments(ARG "" "ABI;VERSION" "SOURCES" ${ARGN})
  set(addon "${target}_${ARG_ABI}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")

  # Create library
  add_nodejs_library(${addon} ${ARG_VERSION}
    LIBRARIES webagent_hosting 
    SOURCES ${ARG_SOURCES}
  )
  add_dependencies(${addon} webagent_hosting)
  set_project_common_output(${addon} "${PROJECT_OUTPUT_DIR}/${target}/addons/${addon}")
  set_target_properties(${addon} PROPERTIES FOLDER "NodeJs/Addon")

  set(${LIST} ${${LIST}} ${addon} PARENT_SCOPE)
endfunction()


# See ABI/Node versions at https://nodejs.org/en/download/releases/
# vABI -> vNodeJs
# 64 -> v10.9.0 
# 67 -> v11.9.0
# 72 -> v12.9.1
# 79 -> v13.9.0
append_group_sources(addon_sources FILTER "*.cpp|*.cc|*.h|*.hpp" DIRECTORIES "./include" "./src_addon")
append_node_webagent_host_ABI_library(addons ${target} ABI 64 VERSION v10.9.0 SOURCES ${addon_sources})
append_node_webagent_host_ABI_library(addons ${target} ABI 67 VERSION v11.9.0 SOURCES ${addon_sources})
append_node_webagent_host_ABI_library(addons ${target} ABI 72 VERSION v12.9.1 SOURCES ${addon_sources})

append_npm_package_json(build
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${PROJECT_OUTPUT_DIR}/${target}
  INSTALL
)

append_npm_build(build
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${PROJECT_OUTPUT_DIR}/${target}
  DIRECTORIES
    "./src"
    "./src/__test__"
)

add_custom_target(${target} ALL DEPENDS ${build} ${addons})
set_target_properties(${target} PROPERTIES FOLDER "NodeJs")

