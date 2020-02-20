set(target "node_webagent_hosting")
set(packageName "node-webagent-hosting")

#----------------------------------------------------------
#-- Node.js Native Addons
#----------------------------------------------------------
# See ABI/Node versions at https://nodejs.org/en/download/releases/
function(append_node_webagent_hosting_ABI_library LIST)
  cmake_parse_arguments(ARG "" "ABI;VERSION" "SOURCES" ${ARGN})
  set(target_addon "node_webagent_hosting_${ARG_ABI}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
  add_nodejs_library(${target_addon} ${ARG_VERSION} LIBRARIES webagent_hosting SOURCES ${ARG_SOURCES})
  add_dependencies(${target_addon} webagent_hosting)
  set_project_common_output(${target_addon} "${PROJECT_OUTPUT_DIR}/${packageName}/addons/${target_addon}")
  set_target_properties(${target_addon} PROPERTIES FOLDER "Hosting/NodeJs")
  set(${LIST} ${${LIST}} ${target_addon} PARENT_SCOPE)
endfunction()
append_group_sources(addon_sources FILTER "*.cpp|*.cc|*.h|*.hpp" DIRECTORIES "./include" "./src_addon")
append_node_webagent_hosting_ABI_library(addons ABI 64 VERSION v10.9.0 SOURCES ${addon_sources})
append_node_webagent_hosting_ABI_library(addons ABI 67 VERSION v11.9.0 SOURCES ${addon_sources})
append_node_webagent_hosting_ABI_library(addons ABI 72 VERSION v12.9.1 SOURCES ${addon_sources})

#----------------------------------------------------------
#-- Node.js script
#----------------------------------------------------------
set(target_js "node_webagent_hosting_js")
append_npm_package_json(build_js ${packageName}
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${PROJECT_OUTPUT_DIR}/${packageName}
  INSTALL
)
append_npm_build(build_js
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${PROJECT_OUTPUT_DIR}/${packageName}
  DIRECTORIES
    "./src"
    "./src/__test__"
)
add_custom_target(${target_js} DEPENDS ${build_js} ${addons})
set_target_properties(${target_js} PROPERTIES FOLDER "Hosting/NodeJs")

#----------------------------------------------------------
#-- Node.js package
#----------------------------------------------------------
append_package_pack(build_package ${packageName} ${PROJECT_OUTPUT_DIR}/${packageName} ${PACKAGES_OUTPUT_DIR})
add_custom_target(${target} ALL DEPENDS ${build_package} ${target_js})
set_target_properties(${target} PROPERTIES FOLDER "Hosting")

