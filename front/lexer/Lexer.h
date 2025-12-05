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
    
    // 缓存机制 
    Token _cachedToken;
    bool _hasCached;
    
    // 词法分析状态
    DFAState DFA[10]; 
    int lineNo;
    std::string currentLine;
    int col;

    // 内部函数
    void initializeDFA();
    int getInputType(char ch);
    Token nextInternal(); 

public:
    Lexer(const std::string& file, SymbolTable* st);
    ~Lexer();

    Token next();
    Token peek();
};