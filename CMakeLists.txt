cmake_minimum_required(VERSION 3.10)
project(CMinusCompiler)

set(CMAKE_CXX_STANDARD 17)

# 1. 包含中端库头文件
include_directories(compiler_ir/include)
include_directories(front)

# 2. 收集源文件
file(GLOB_RECURSE FRONT_SRC "front/*.cpp")
file(GLOB_RECURSE MIDDLE_SRC "compiler_ir/src/*.cpp")

# 3. 生成可执行文件
add_executable(compiler main.cpp ${FRONT_SRC} ${MIDDLE_SRC})
