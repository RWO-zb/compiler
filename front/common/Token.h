#pragma once
#include <string>

// [Role D] TODO: 必须确保这里的枚举值能覆盖 C-- 文法的所有 Terminal
// A:根据词法要求去除了while加了mod等几个符号，其他符号命名可能有修改。
// 另，词法分析器没要求'!'但文法中要用所以加了一下OP_NOT, lexer代码大概也需要相应识别'!'
enum TokenType {
    ID, INT_CONST, FLOAT_CONST,
    KW_INT, KW_VOID, KW_RETURN, KW_CONST, KW_MAIN, KW_FLOAT, KW_IF, KW_ELSE, 
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_MOD, OP_ASSIGN, 
    OP_NOT, OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE, OP_AND, OP_OR, 
    SE_LPAREN, SE_RPAREN, SE_LBRACE, SE_RBRACE, SE_SEMICOLON, SE_COMMA,
    END_OFF // 文件结束符
};

struct Token {
    TokenType type;
    std::string content; // 原始文本
    int line;            // 行号
    // 可选：int intVal; float floatVal;
};