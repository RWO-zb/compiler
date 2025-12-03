#pragma once
#include "../common/Token.h"
#include "../common/SymbolTable.h"
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

// DFA 状态定义
struct DFAState {
    std::unordered_map<int, int> transitions;
    TokenType type;
};

class Lexer {
private:
    std::string filename;
    SymbolTable* symTable;
    std::ifstream file;
    
    // 缓存机制 (用于 peek)
    Token _cachedToken;
    bool _hasCached;
    
    // 词法分析状态
    DFAState DFA[10]; // 根据 LexicalAnaly.cpp 的状态数量调整
    int lineNo;
    std::string currentLine;
    int col;

    // 内部函数
    void initializeDFA();
    int getInputType(char ch);
    Token nextInternal(); // 实际的扫描逻辑

public:
    Lexer(const std::string& file, SymbolTable* st);
    ~Lexer();

    Token next();
    Token peek();
};