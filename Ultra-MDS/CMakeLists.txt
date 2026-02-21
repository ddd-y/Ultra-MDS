# CMakeList.txt: Ultra-MDS 的 CMake 项目


cmake_minimum_required(VERSION 3.16)
project(Ultra-MDS CXX)

# ========== Protobuf 配置 ==========
find_package(Protobuf 3.21.12 EXACT REQUIRED)
message(STATUS "========================================")
message(STATUS "✅ Protobuf 找到！")
message(STATUS "   版本: ${Protobuf_VERSION}")
message(STATUS "   头文件路径: ${Protobuf_INCLUDE_DIRS}")
message(STATUS "   库文件: ${Protobuf_LIBRARIES}")
message(STATUS "========================================")

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
    "DataHandler/detector.h"  
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
target_link_libraries(my_logger PUBLIC spdlog::spdlog fmt::fmt)
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
    protobuf
    ${Boost_LIBRARIES}
)

# ========== 日志和配置文件同步 ==========
add_definitions(-DDEFAULT_LOG_PATH="${CMAKE_CURRENT_BINARY_DIR}/logs/test_log.txt")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/logs")
add_custom_command(TARGET Ultra-MDS PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/logs"
    COMMENT "Ensuring logs directory exists..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/myconfig.json
        ${CMAKE_CURRENT_BINARY_DIR}/myconfig.json
    COMMENT "Syncing config file..."
)

