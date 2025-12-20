#pragma once
#include <string>

// 定义 Token 类型
enum TokenType {
    // 标识符与常量
    ID, INT_CONST, FLOAT_CONST,
    
    // 关键字 
    KW_INT, KW_VOID, KW_RETURN, KW_IF, KW_ELSE, 
    KW_FLOAT, KW_CONST, KW_MAIN, 
    
    // 运算符 
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_ASSIGN, 
    OP_MOD, // %
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,
    OP_AND, OP_OR, OP_NOT, // &&, ||, !
    
    // 界符
    SE_LPAREN, SE_RPAREN, SE_LBRACE, SE_RBRACE, SE_SEMICOLON, SE_COMMA,
    
    // 结束符
    END_OFF 
};

struct Token {
    TokenType type;
    std::string content;
    int line;
};