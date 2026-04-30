cmake_minimum_required(VERSION 3.28)
project(client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -fsanitize=address -fsanitize=undefined -g -Wall -Wextra")
    message(STATUS "Selected ${CMAKE_BUILD_TYPE} build type. Building with sanitizers.")

    find_program(CPPCHECK cppcheck)
    if(CPPCHECK)
        add_custom_target(cppcheck
                COMMAND ${CPPCHECK}
                --enable=all
                --suppress=missingIncludeSystem
                --suppress=unmatchedSuppression
                --std=c++20
                --verbose
                --error-exitcode=1
                ${CMAKE_SOURCE_DIR}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running cppcheck static analysis"
        )
        message(STATUS "cppcheck found: ${CPPCHECK}")
    else()
        message(WARNING "cppcheck not found - static analysis disabled")
    endif()
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O3")
endif ()

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(${PROJECT_NAME}
        context/Context.cpp
        context/DataStorage.cpp
        exchange/Exchange.cpp
        exchange/ReadPacket.cpp
        menu/Menu.cpp
        menu/MenuItem.cpp
        ../commonFiles/byteFunc/BytesTransform.cpp
        ../commonFiles/resultFunc/ResultFunction.cpp
        ../commonFiles/validateFunc/ValidateFunc.hpp
        main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/context
        ${CMAKE_CURRENT_SOURCE_DIR}/exchange
        ${CMAKE_CURRENT_SOURCE_DIR}/menu
        ${CMAKE_CURRENT_SOURCE_DIR}/../commonFiles/
)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)