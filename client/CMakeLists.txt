cmake_minimum_required(VERSION 3.28)
project(client)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(${PROJECT_NAME}
        ./context/context.cpp
        ./exchange/exchange.cpp
        ./menu/menu.cpp
        ../commonFiles/byteFunc/BytesTransform.cpp
        ../commonFiles/resultFunc/ResultFunction.cpp
        main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/context
        ${CMAKE_CURRENT_SOURCE_DIR}/exchange
        ${CMAKE_CURRENT_SOURCE_DIR}/menu
        ${CMAKE_CURRENT_SOURCE_DIR}/byteFunc
        ${CMAKE_CURRENT_SOURCE_DIR}/resultFunc
        ${CMAKE_CURRENT_SOURCE_DIR}/../commonFiles/
)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)