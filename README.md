# compiler
CMinusCompiler/
├── CMakeLists.txt                  # [Role D] 构建脚本，负责集成编译
├── main.cpp                        # [Role D] 主程序入口，串联 Lexer -> Parser -> IRGen
├── grammar.txt                     # [Role A] 文法定义文件 (清洗并确认无误)
│
├── compiler_ir/                    # [Role C 重点参考] 助教提供的中端库 (已提供，无需修改)
│   ├── include/                    # (IRBuilder.h, Module.h, Value.h 等)
│   └── src/                        # (对应实现)
│
└── front/                          # [核心开发区] 前端代码
    ├── common/                     # [公共定义]
    │   ├── Token.h                 # [Role D] Token 结构定义
    │   ├── SymbolTable.h           # [Role D] 符号表类定义
    │   └── Errors.h                # [Role D] 错误处理类
    │
    ├── lexer/                      # [Role D ] 词法分析
    │   ├── Lexer.h                 # [Role D] 词法分析器接口
    │   ├── Lexer.cpp               # [Role D] 封装原 LexicalAnaly.cpp/nfa2dfa.cpp
    │   └── ... (原自动机相关代码)
    │
    ├── syntax/                     # [Role A & B ] 语法分析
    │   ├── SLRGenerator.h          # [Role A] SLR 表生成器接口
    │   ├── SLRGenerator.cpp        # [Role A] 封装原 SLR.cpp/closure.cpp/first_follow.cpp
    │   ├── Parser.h                # [Role B] 语法分析驱动接口
    │   └── Parser.cpp              # [Role B] 修改原 SyntaxAnaly.cpp，增加 AST 构建逻辑
    │
    ├── ast/                        # [Role B & C ] 抽象语法树
    │   └── AST.h                   # [Role B 定义结构, Role C 确认接口] AST 节点类
    │
    └── codegen/                    # [Role C ] 中间代码生成
        ├── IRGenerator.h           # [Role C] Visitor 模式接口
        └── IRGenerator.cpp         # [Role C] 核心：遍历 AST 调用 compiler_ir 生成代码