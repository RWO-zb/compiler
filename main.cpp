#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "compiler_ir/include/Module.h"
#include "front/common/SymbolTable.h"
#include "front/lexer/Lexer.h"
#include "front/syntax/SLRGenerator.h"
#include "front/syntax/Parser.h"
#include "front/codegen/IRGenerator.h"

// 辅助函数：根据用户提供的规则输出 Token
void printToken(const Token& t) {
    std::string typeStr;
    std::string attr;

    // === 1. 关键字映射 (1-8) ===
    static std::map<std::string, int> kwMap = {
        {"int", 1}, {"void", 2}, {"return", 3}, {"const", 4},
        {"main", 5}, {"float", 6}, {"if", 7}, {"else", 8}
    };

    // === 2. 运算符映射 (9-22) ===
    static std::map<std::string, int> opMap = {
        {"+", 9},   {"-", 10},  {"*", 11},  {"/", 12},  {"%", 13},
        {"=", 14},  {">", 15},  {"<", 16},  {"==", 17}, {"<=", 18},
        {">=", 19}, {"!=", 20}, {"&&", 21}, {"||", 22}
    };

    // === 3. 界符映射 (23-28) ===
    static std::map<std::string, int> seMap = {
        {"(", 23}, {")", 24}, {"{", 25}, {"}", 26}, {";", 27}, {",", 28}
    };

    switch (t.type) {
        // --- 关键字 ---
        case KW_INT: case KW_VOID: case KW_RETURN: case KW_CONST:
        case KW_MAIN: case KW_FLOAT: case KW_IF: case KW_ELSE:
            typeStr = "KW";
            if (kwMap.count(t.content)) attr = std::to_string(kwMap[t.content]);
            else attr = "0"; 
            break;

        // --- 标识符 ---
        case ID:
            typeStr = "IDN";
            attr = t.content; // 属性为标识符本身
            break;

        // --- 常量 ---
        case INT_CONST:
            typeStr = "INT";
            attr = t.content; // 属性为数值字符串
            break;
        case FLOAT_CONST:
            typeStr = "FLOAT";
            attr = t.content;
            break;

        // --- 运算符 ---
        case OP_PLUS: case OP_MINUS: case OP_MUL: case OP_DIV: case OP_MOD:
        case OP_ASSIGN: case OP_EQ: case OP_NEQ: case OP_LT: case OP_GT:
        case OP_LE: case OP_GE: case OP_AND: case OP_OR:
            typeStr = "OP";
            if (opMap.count(t.content)) attr = std::to_string(opMap[t.content]);
            else attr = "?";
            break;
        
        case OP_NOT: 
            typeStr = "OP"; 
            attr = "?"; 
            break;

        // --- 界符 ---
        case SE_LPAREN: case SE_RPAREN: case SE_LBRACE: case SE_RBRACE:
        case SE_SEMICOLON: case SE_COMMA:
            typeStr = "SE";
            if (seMap.count(t.content)) attr = std::to_string(seMap[t.content]);
            else attr = "?";
            break;

        default:
            return; // 忽略 END_OFF 或其他
    }

    // 输出格式：[内容] <[类别], [属性]>
    std::cout << t.content << "\t<" << typeStr << ", " << attr << ">" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./compiler <source_file>" << std::endl;
        return 1;
    }
    std::string sourceFile = argv[1];

    // 1. 初始化符号表
    SymbolTable symTable; 

    // === 任务 1: 输出词法分析结果 ===
    // 使用一个临时的 Lexer 遍历并打印所有 Token
    {
        Lexer tempLexer(sourceFile, &symTable);
        while (true) {
            Token t = tempLexer.next();
            if (t.type == END_OFF) break;
            printToken(t);
        }
    }

    // 2. 准备 SLR 分析表
    // 重新创建 Lexer 给 Parser 使用（因为之前的遍历已经消耗了文件流）
    Lexer lexer(sourceFile, &symTable); 
    SLRGenerator slrGen;
    
    slrGen.build(); 

    // 3. 语法分析 & 构建 AST & 输出归约过程
    // Parser 内部已修改为打印归约序列
    Parser parser(lexer, slrGen);
    ASTNode* root = parser.parse();

    if (!root) {
        // 解析失败，通常 Parser 内部已经打印了部分步骤
        return 1; 
    }

    // 4. 中间代码生成
    Module module("sysy2022_compiler"); 
    IRGenerator irGen(&module, &symTable);
    
    root->accept(irGen); 

    // 5. 输出最终 IR (带题目要求的头部)
    std::cout << "; ModuleID = 'sysy2022_compiler'" << std::endl;
    std::cout << "source_filename = \"" << sourceFile << "\"" << std::endl;
    std::cout << module.print() << std::endl;

    return 0;
}