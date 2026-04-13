cmake_minimum_required(VERSION 3.28)
project(mobile_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server_app
        main.cpp
        baseStation/BaseStation.cpp
        listener/Listener.cpp
        ueContext/UeContext.cpp
        ../commonFiles/resultFunc/ResultFunction.cpp
        ueContext/HandleMessage.cpp
        ueContext/HandleMessage.hpp
        ueContext/BytesTransform.cpp
        ueContext/BytesTransform.hpp
        mme/MME.cpp
        mme/MME.hpp
        "  smsc/SMSC.cpp"
        "  smsc/SMSC.h"
)

target_include_directories(server_app PRIVATE
        .
        baseStation
        listener
        ueContext
        ../commonFiles/resultFunc
)