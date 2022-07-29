cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED True)

project (discord_voice)
include_directories(${CMAKE_JS_INC})
add_library(${PROJECT_NAME} SHARED "engine.cpp" ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE NODE_ADDON_API_DIR
	)
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

add_subdirectory("./pipewire")

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=3)

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} PRIVATE discord_voice_pipewire)
