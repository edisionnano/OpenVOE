cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_FLAGS " -fPIC")
set(CMAKE_CXX_FLAGS " -fPIC")

project (discord_voice)
include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES "engine.cpp")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Fetch rohrkabel
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(rohrkabel GIT_REPOSITORY "https://github.com/Soundux/rohrkabel")
    FetchContent_MakeAvailable(rohrkabel)
endif()

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=3)

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} PRIVATE rohrkabel)
