#pragma once
#include "../common/Token.h"
#include "../common/SymbolTable.h"
#include <string>
#include <vector>

// [Role D] 负责人：适配原 LexicalAnaly.cpp
class Lexer {
private:
    std::string filename;
    SymbolTable* symTable;
    // [Role D] TODO: 这里可能需要原 automatic 机相关的状态变量

public:
    Lexer(const std::string& file, SymbolTable* st) : filename(file), symTable(st) {
        // [Role D] TODO: 在这里调用原代码的初始化逻辑 (如 nfa2dfa)
    }

    // 核心接口
    Token next() {
        // [Role D] TODO: 改造原 LexicalAnaly.cpp 的扫描逻辑
        // 每次调用返回一个 Token，而不是直接写入文件
        // 遇到 ID 时，暂时不需要往 SymbolTable 填 Value，只需返回 ID Token
        return {END_OFF, "", 0}; 
    }
    
    // 辅助：预读当前 Token (不消耗)
    Token peek() {
        // [Role D] TODO: 实现预读
        return {END_OFF, "", 0};
    }
};