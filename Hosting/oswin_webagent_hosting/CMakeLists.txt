set(target "oswin_webagent_hosting")

# Create library
add_executable(${target} WIN32 "main.cpp")
add_dependencies(${target} webagent_hosting)
set_project_common_output(${target} "${PROJECT_OUTPUT_DIR}/bin")
set_target_properties(${target} PROPERTIES FOLDER "Hosting")

# Set link properties
target_link_libraries(${target} PUBLIC general webagent_hosting)
target_link_options(${target} PRIVATE /SUBSYSTEM:CONSOLE)
