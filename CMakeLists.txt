cmake_minimum_required(VERSION 3.22)

# 项目信息
project(ModbusTcp)

# 添加头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置源文件
# set(SOURCES
#     testModbusTcp.cpp # 执行文件
#     ModbusTcpServer.cpp
# )
set(SOURCES
    testMTMaster.cpp # 执行文件
    ModbusTcpMaster.cpp
)

# 指定生成目标
add_executable(run ${SOURCES})

# 加入多个链接库
target_link_libraries(run modbus)