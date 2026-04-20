cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tests)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME}
        BaseStationTest.cpp
        ListenerTest.cpp
        MMETest.cpp
        RegisterTest.cpp
        SMSCTest.cpp
        UeContextTest.cpp

        ../baseStation/BaseStation.cpp
        ../ueContext/UeContext.cpp
        ../ueContext/HandleMessage.cpp
        ../mme/MME.cpp
        ../listener/Listener.cpp
        ../smsc/SMSC.cpp
        ../register/Register.cpp
        ../../commonFiles/resultFunc/ResultFunction.cpp
        ../../commonFiles/byteFunc/BytesTransform.cpp
        ../../commonFiles/utilityFunc/UtilityFunc.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        gtest_main
        SQLite::SQLite3
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
#        ${CMAKE_CURRENT_SOURCE_DIR}/../baseStation
#        ${CMAKE_CURRENT_SOURCE_DIR}/../listener
#        ${CMAKE_CURRENT_SOURCE_DIR}/../ueContext
#        ${CMAKE_CURRENT_SOURCE_DIR}/../mme
#        ${CMAKE_CURRENT_SOURCE_DIR}/../smsc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../register
#        ${CMAKE_CURRENT_SOURCE_DIR}/../../commonFiles
#        ${CMAKE_CURRENT_SOURCE_DIR}/../config
        ${CMAKE_CURRENT_SOURCE_DIR}/..

)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
