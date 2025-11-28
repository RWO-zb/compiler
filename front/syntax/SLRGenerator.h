#pragma once
#include <map>
#include <string>
#include <vector>
#include "../common/Token.h"

// 动作表项
struct Action {
    enum Type { SHIFT, REDUCE, ACCEPT, ERROR } type;
    int target; // 状态ID 或 产生式ID
};

// 产生式信息
struct Production {
    int id;
    std::string lhs; // 左部非终结符
    int rhsLen;      // 右部长度 (用于规约时弹出栈)
};

// [Role A] 负责人：移植原 SLR.cpp 等算法
class SLRGenerator {
private:
    std::map<int, std::map<TokenType, Action>> actionTable;
    std::map<int, std::map<std::string, int>> gotoTable;
    std::vector<Production> productions;

public:
    void build() {
        // [Role A] TODO: 
        // 1. 调用 loadGrammar("grammar.txt");
        // 2. 调用 computeFirstFollow();
        // 3. 调用 buildLR0();
        // 4. 调用 buildSLRTable();
    }
    
    // 提供给 Parser 的接口
    Action getAction(int state, TokenType type) {
        if (actionTable[state].count(type)) return actionTable[state][type];
        return {Action::ERROR, 0};
    }
    
    int getGoto(int state, const std::string& nonTerminal) {
        if (gotoTable[state].count(nonTerminal)) return gotoTable[state][nonTerminal];
        return -1;
    }
    
    Production getProduction(int id) {
        return productions[id]; // [Role A] TODO: 确保 id 合法
    }
};