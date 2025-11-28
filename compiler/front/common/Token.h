#pragma once
#include <string>

// [Role D] TODO: 必须确保这里的枚举值能覆盖 C-- 文法的所有 Terminal
enum TokenType {
    ID, INT_CONST, FLOAT_CONST,
    KW_INT, KW_VOID, KW_RETURN, KW_IF, KW_ELSE, KW_WHILE,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_ASSIGN, 
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,
    SE_LPAREN, SE_RPAREN, SE_LBRACE, SE_RBRACE, SE_SEMICOLON, SE_COMMA,
    END_OFF // 文件结束符
};

struct Token {
    TokenType type;
    std::string content; // 原始文本
    int line;            // 行号
    // 可选：int intVal; float floatVal;
};