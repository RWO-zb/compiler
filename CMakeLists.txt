cmake_minimum_required(VERSION 3.10)
project(CMinusCompiler)

set(CMAKE_CXX_STANDARD 17)

# 1. 包含中端库头文件
include_directories(compiler_ir/include)
include_directories(front)

# 2. 收集源文件
# [Role D] 注意：如果有新增文件，确保它们被包含进来
file(GLOB_RECURSE FRONT_SRC "front/*.cpp")
file(GLOB_RECURSE MIDDLE_SRC "compiler_ir/src/*.cpp")

# 3. 生成可执行文件
add_executable(compiler main.cpp ${FRONT_SRC} ${MIDDLE_SRC})

# 4. 链接库 (如果中端库使用了线程等)
# target_link_libraries(compiler pthread)