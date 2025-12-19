cmake_minimum_required(VERSION 3.10)
project(CMinusCompiler)

set(CMAKE_CXX_STANDARD 17)

# === 新增配置开始 ===
# 针对 MSVC 编译器的特定设置
if(MSVC)
    # 1. 解决 C4819 编码警告：强制使用 UTF-8 编码
    add_compile_options(/utf-8)
    
    # 2. (可选) 抑制 C4267 (size_t -> int 转换丢失数据) 等常见警告，让输出更干净
    # 如果你想严格检查类型，可以注释掉下面这行
    add_compile_options(/wd4267)
endif()

# 3. [关键修复] 添加项目根目录到包含路径
# 解决 error C1083: 无法打开包括文件: “compiler_ir/include/Module.h”
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# === 新增配置结束 ===

# 原有的包含路径
include_directories(compiler_ir/include)
include_directories(front)

# 2. 收集源文件
file(GLOB_RECURSE FRONT_SRC "front/*.cpp")
file(GLOB_RECURSE MIDDLE_SRC "compiler_ir/src/*.cpp")

# 3. 生成可执行文件
add_executable(compiler main.cpp ${FRONT_SRC} ${MIDDLE_SRC})