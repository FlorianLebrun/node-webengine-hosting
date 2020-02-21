set(target "webagent_hosting")
add_library(${target} STATIC "./src/webx.cpp")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
