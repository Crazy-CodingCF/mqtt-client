#
# mkdir build
# cd build
# For x64 compile, please use command
# cmake ..
# For arm cross compile, please use command
# cmake -DPLATFORM=arm ..
# make
#
cmake_minimum_required(VERSION 3.5)   # CMake version check

project(mqtt_alink)

#if not set, then will use log
option(USE_LOG "select to use log print" ON)
option(VERSION_MAJOR "select to use log print" 1)
option(VERSION_MINOR "select to use log print" 0)
option(VERSION_RELEASE "select to use log print" 0)

#select to use specified function, or common function
message("-- USE_LOG is ${USE_LOG}")
message("-- VERSION_MAJOR   ${VERSION_MAJOR}")
message("-- VERSION_MINOR  ${VERSION_MINOR}")
message("-- VERSION_RELEASE  ${VERSION_RELEASE}")

if(PLATFORM MATCHES "arm")
    if(USE_LOG)
        add_definitions(-DUSE_LOG)
    endif()

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)

    set(CMAKE_CXX_COMPILER /home/dev/Compiler_tools/arm-2014.05/bin/arm-none-linux-gnueabi-g++)
    set(CMAKE_C_COMPILER   /home/dev/Compiler_tools/arm-2014.05/bin/arm-none-linux-gnueabi-gcc)
    set(GNU_FLAGS "-mfpu=vfp -fPIC")
    set(GNU_FLAGS "-std=c++0x -W -s")   # -W 表示增强语法检测， -s表示删除所有符号，所见程序大小
    set(CMAKE_CXX_FLAGS "${GNU_FLAGS}")
    set(CMAKE_C_FLAGS   "${GNU_FLAGS}")

    include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/mqtt/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/glog/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/jsoncpp/include)

    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/mqtt/lib)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/glog/lib)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm/jsoncpp/lib)
else()
    if(USE_LOG)
        add_definitions(-DUSE_LOG)
    endif()

    set(GNU_FLAGS "-std=c++0x -W")
    set(CMAKE_CXX_FLAGS "${GNU_FLAGS}")
    set(CMAKE_C_FLAGS   "${GNU_FLAGS}")

    include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/mqtt/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/glog/include)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/jsoncpp/include)

    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/mqtt/lib)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/glog/lib)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/x64/jsoncpp/lib)
endif()


#通过cmake配置文件获取宏并生成头文件
configure_file(
    "${PROJECT_SOURCE_DIR}/script/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    )

include_directories("${PROJECT_BINARY_DIR}")

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src ALL_SRCS_MAIN)

add_executable(mqtt_alink ${ALL_SRCS_MAIN})

add_library(mqtt1.1 SHARED ${ALL_SRCS_MAIN})

set_property(TARGET mqtt_alink PROPERTY CXX_STANDARD 11)
set_property(TARGET mqtt_alink PROPERTY CXX_STANDARD_REQUIRED ON)

#target_link_libraries(mqtt_alink paho-mqtt3as glog jsoncpp pthread rt ssl crypto)
#target_link_libraries(mqtt1.1 paho-mqtt3as glog jsoncpp pthread rt ssl  crypto)

if(PLATFORM MATCHES "arm")
    target_link_libraries(mqtt_alink paho-mqtt3as glog pthread rt ssl  crypto
        ${CMAKE_CURRENT_LIST_DIR}/libs/arm/jsoncpp/lib/libjsoncpp.a
        )
else()
    target_link_libraries(mqtt_alink paho-mqtt3as glog pthread rt ssl  crypto
        ${CMAKE_CURRENT_LIST_DIR}/libs/x64/jsoncpp/lib/libjsoncpp.a
        )
endif()

if(PLATFORM MATCHES "arm")
    target_link_libraries(mqtt1.1 paho-mqtt3as glog pthread rt ssl  crypto
        ${CMAKE_CURRENT_LIST_DIR}/libs/arm/jsoncpp/lib/libjsoncpp.a
        )
else()
    target_link_libraries(mqtt1.1 paho-mqtt3as glog pthread rt ssl  crypto
        ${CMAKE_CURRENT_LIST_DIR}/libs/x64/jsoncpp/lib/libjsoncpp.a
        )
endif()

#通过脚本文件获取
execute_process(COMMAND chmod 777 ../script/version.sh)
execute_process(COMMAND ../script/version.sh OUTPUT_VARIABLE VERSION_INFO)
MESSAGE("-- MQTT VERSION : ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")
SET_TARGET_PROPERTIES(mqtt1.1 PROPERTIES VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE} SOVERSION 1)
