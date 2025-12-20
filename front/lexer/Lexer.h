#pragma once
#include "../common/Token.h"
#include "../common/SymbolTable.h"
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

class Lexer {
private:
    std::string filename;
    SymbolTable* symTable;
    std::ifstream file;
    
    // 缓存机制 
    Token _cachedToken;
    bool _hasCached;
    
    // 行列号管理
    int lineNo;
    std::string currentLine;
    int col;

    // --- DFA 状态定义 ---
    enum State {
        START,              // 初始状态
        IN_ID,              // 标识符
        IN_INT,             // 整数
        IN_FLOAT_DOT,       // 浮点数的小数点态 (123.)
        IN_FLOAT,           // 浮点数 (123.45)
        IN_EQ,              // =
        IN_GT,              // >
        IN_LT,              // <
        IN_NOT,             // !
        IN_AND,             // &
        IN_OR,              // |
        IN_COMMENT_START,   // / (可能是除号，也可能是注释开始)
        IN_LINE_COMMENT,    // //
        IN_BLOCK_COMMENT,   // /*
        IN_BLOCK_COMMENT_END,// /* ... * (准备结束)
        DONE                // 完成状态
    };

    // 内部辅助函数
    char getChar();         // 获取下一个字符
    void retract();         // 回退一个字符
    Token nextInternal();   // 核心 DFA 驱动函数

public:
    Lexer(const std::string& file, SymbolTable* st);
    ~Lexer();

    Token next();
    Token peek();
};