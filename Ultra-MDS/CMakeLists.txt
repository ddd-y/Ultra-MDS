# CMakeList.txt: Ultra-MDS 的 CMake 项目


cmake_minimum_required(VERSION 3.16)
project(Ultra-MDS CXX)


# ========== Protobuf 配置 ==========
find_package(Protobuf REQUIRED)
message(STATUS "✅ Protobuf 找到！版本: ${Protobuf_VERSION}")

# 定义路径：直接指向源码里的 SharedDefine
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SharedDefine")
set(PROTO_FILE "${PROTO_SRC_DIR}/tick_data.proto")

# 直接调用 protoc 命令，生成对应的文件
add_custom_command(
    OUTPUT 
        ${PROTO_SRC_DIR}/tick_data.pb.h 
        ${PROTO_SRC_DIR}/tick_data.pb.cc
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS 
        --cpp_out=${PROTO_SRC_DIR}  # 直接输出到源码的 SharedDefine！！！
        --proto_path=${PROTO_SRC_DIR}
        tick_data.proto
    DEPENDS ${PROTO_FILE}
    COMMENT "🔧 正在将 Protobuf 文件生成到源码目录: ${PROTO_SRC_DIR}"
)
message(STATUS "========================================")
message(STATUS "✅ Protobuf 找到！")
message(STATUS "   版本: ${Protobuf_VERSION}")
message(STATUS "   头文件路径: ${Protobuf_INCLUDE_DIRS}")
message(STATUS "   库文件: ${Protobuf_LIBRARIES}")
message(STATUS "========================================")

# 核心：只指向【源码目录的SharedDefine】（你要的目标目录，全程不碰构建目录）
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SharedDefine")  # 就是你图里的SharedDefine
set(PROTO_FILE "${PROTO_DIR}/tick_data.proto")             # 源码里的proto文件

# 关键：指定protoc的--proto_path，解决生成的.pb.cc里#include带"SharedDefine/"前缀的致命问题
set(PROTOBUF_PROTOC_OPTIONS "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Boost REQUIRED COMPONENTS system)

# 打印 Boost 信息
message(STATUS "========================================")
message(STATUS "✅ Boost 找到！")
message(STATUS "   版本: ${Boost_VERSION}")
message(STATUS "   头文件路径: ${Boost_INCLUDE_DIRS}")
message(STATUS "   库文件: ${Boost_LIBRARIES}")
message(STATUS "========================================")

# ========== 将源代码添加到此项目的可执行文件 ==========
add_executable (Ultra-MDS 
    "Ultra-MDS.cpp" 
    "Ultra-MDS.h"  
    "CtpRelated/MarketSPI.h" 
    "CtpRelated/MarketSPI.cpp" 
    "Tool/SPSCQueue.h" 
    "DataHandler/UnitProcessor.h"  
    "configrelated/Loginer.h"   
    "CtpRelated/Contract/Dispatcher.h" 
    "DataHandler/TickHandler.h"  
    "DataHandler/TickHandler.cpp"  
    "SharedDefine/TickData.h" 
    "SystemStart.h"   
    "CtpRelated/Contract/Container.h"        
    "configrelated/ConfigRead.h" 

    "SharedDefine/tick_data.pb.h"
    "SharedDefine/tick_data.pb.cc"
    "test/tick_perf_test.h" )



# 编译器优化配置：同时支持 Windows MSVC 和 Linux GCC/Clang
if(MSVC)
    # Windows MSVC 优化配置
    target_compile_options(Ultra-MDS PRIVATE
        $<$<CONFIG:Release>:/O2 /GL>
        $<$<CONFIG:RelWithDebInfo>:/O2 /GL>
    )
    target_link_options(Ultra-MDS PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/LTCG>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Linux GCC/Clang 优化配置
    target_compile_options(Ultra-MDS PRIVATE
        # Release模式：最高级速度优化 + 链接时优化 + 原生CPU指令集
        $<$<CONFIG:Release>:-O3 -flto -march=native>
        # RelWithDebInfo模式
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -flto -march=native>
        # Debug模式：关闭优化 + 完整调试符号
        $<$<CONFIG:Debug>:-O0 -g>
    )

    # 链接选项：配合-flto实现跨文件全程序优化
    target_link_options(Ultra-MDS PRIVATE
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:RelWithDebInfo>:-flto>
    )
endif()

# C++ 标准设置
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Ultra-MDS PROPERTY CXX_STANDARD 20)
endif()

# ========== spdlog 配置 ==========
find_package(spdlog REQUIRED)
add_library(my_logger STATIC 
    "../Log/Logger.h"
    "../Log/Logger.cpp")
target_link_libraries(my_logger PUBLIC spdlog::spdlog)
target_include_directories(my_logger PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../Log")

# ========== 头文件包含目录 ==========
target_include_directories(Ultra-MDS PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/Tool"
    "${CMAKE_CURRENT_SOURCE_DIR}/SharedDefine"
    "${CMAKE_CURRENT_SOURCE_DIR}/DataHandler"
    "${CMAKE_CURRENT_SOURCE_DIR}/CtpRelated"
    "${CMAKE_CURRENT_SOURCE_DIR}/configrelated"
    "${CMAKE_CURRENT_SOURCE_DIR}/ctp/include"
    ${Protobuf_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# ========== 链接目录 ==========
target_link_directories(Ultra-MDS PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/test"
    "${CMAKE_CURRENT_SOURCE_DIR}/ctp/lib"
)

# ========== 链接库 ==========
target_link_libraries(Ultra-MDS PRIVATE 
    my_logger
    "thostmduserapi_se"
    "thosttraderapi_se"
    pthread
    ${Protobuf_LIBRARIES}  
    ${Boost_LIBRARIES}
)

# ========== 日志和配置文件同步 ==========
add_definitions(-DDEFAULT_LOG_PATH="${CMAKE_CURRENT_BINARY_DIR}/logs/test_log.txt")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/logs")
add_custom_command(TARGET Ultra-MDS PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/logs"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/myconfig.json
        ${CMAKE_CURRENT_BINARY_DIR}/myconfig.json
    COMMENT "Ensuring logs directory exists and syncing config file..."
)

