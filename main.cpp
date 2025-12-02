// [Role D] 负责人
#include <iostream>
#include "compiler_ir/include/Module.h"
#include "front/common/SymbolTable.h"
#include "front/lexer/Lexer.h"
#include "front/syntax/SLRGenerator.h"
#include "front/syntax/Parser.h"
#include "front/codegen/IRGenerator.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./compiler <source_file>" << std::endl;
        return 1;
    }
    std::string sourceFile = argv[1];

    // 1. 初始化基础模块
    SymbolTable symTable; // 符号表
    Lexer lexer(sourceFile, &symTable); // 词法分析器

    // 2. 准备 SLR 分析表 (Role A)
    SLRGenerator slrGen;
    slrGen.build(); // 加载文法并生成表

    // 3. 语法分析 & 构建 AST (Role B)
    Parser parser(lexer, slrGen);
    ASTNode* root = parser.parse();

    if (!root) {
        std::cerr << "Parsing failed." << std::endl;
        return 1;
    }

    // 4. 中间代码生成 (Role C)
    Module module("main_module"); // 中端库的模块
    IRGenerator irGen(&module, &symTable);
    
    root->accept(irGen); // 遍历 AST 生成 IR

    // 5. 输出最终 IR
    std::cout << module.print() << std::endl;

    // 清理内存 (可选)
    // delete root;
    return 0;
}