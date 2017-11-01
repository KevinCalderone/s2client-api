include_directories("../include")
include_directories("common")

file(GLOB common_sources "common/*.h" "common/*.cc")

include_directories(SYSTEM "${PROJECT_BINARY_DIR}/generated")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/contrib/SDL-mirror/include")

# Setup MSVC parallelized builds
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Exclude SDL and related projects to work around linker issue.
if (NOT APPLE)
    #rendered
    add_executable(rendered "rendered.cc")
    set_target_properties(rendered PROPERTIES FOLDER examples)
    target_link_libraries(rendered sc2api sc2utils sc2renderer)

    #rendered_actions
    add_executable(rendered_actions "rendered_actions.cc")
    set_target_properties(rendered_actions PROPERTIES FOLDER examples)
    target_link_libraries(rendered_actions sc2api sc2utils sc2renderer)
endif ()

#annoying_helper
add_executable(save_load "save_load.cc")
set_target_properties(save_load PROPERTIES FOLDER examples)
target_link_libraries(save_load sc2api sc2lib sc2utils)
