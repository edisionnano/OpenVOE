project(discord_voice_pipewire)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PipeWire REQUIRED)

file(GLOB SOURCES "*.cpp")

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE NODE_ADDON_API_DIR
	)
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

add_library(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON )
target_include_directories(${PROJECT_NAME} PUBLIC "./" PRIVATE ${NODE_ADDON_API_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC PipeWire::PipeWire)
