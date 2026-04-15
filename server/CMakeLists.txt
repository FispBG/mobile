cmake_minimum_required(VERSION 3.28)
project(server)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(${PROJECT_NAME}
        main.cpp
        baseStation/BaseStation.cpp
        listener/Listener.cpp
        ueContext/UeContext.cpp
        ../commonFiles/resultFunc/ResultFunction.cpp
        ueContext/HandleMessage.cpp
        ../commonFiles/byteFunc/BytesTransform.cpp
        mme/MME.cpp
        smsc/SMSC.cpp
        register/Register.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/baseStation
        ${CMAKE_CURRENT_SOURCE_DIR}/listener
        ${CMAKE_CURRENT_SOURCE_DIR}/ueContext
        ${CMAKE_CURRENT_SOURCE_DIR}/mme
        ${CMAKE_CURRENT_SOURCE_DIR}/smsc
        ${CMAKE_CURRENT_SOURCE_DIR}/register
        ${CMAKE_CURRENT_SOURCE_DIR}/../commonFiles/resultFunc
        ${CMAKE_CURRENT_SOURCE_DIR}/config
)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)