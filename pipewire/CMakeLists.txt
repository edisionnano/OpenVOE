project(discord_voice_pipewire)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PipeWire REQUIRED)

add_library(${PROJECT_NAME} "device_change.cpp")
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON )
target_include_directories(${PROJECT_NAME} PUBLIC "./")
target_link_libraries(${PROJECT_NAME} PUBLIC PipeWire::PipeWire)
