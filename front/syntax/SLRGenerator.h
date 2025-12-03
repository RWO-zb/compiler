#pragma once
// ===============================================
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>  // 解决 ifstream 报错
#include <sstream>  // 解决 stringstream 报错
#include <utility>  // 解决 pair 报错
// ===============================================
#include "../common/Token.h"
using namespace std;

// 动作表项
struct Action {
    enum Type { SHIFT, REDUCE, ACCEPT, ERROR } type;
    int target; // 状态ID 或 产生式ID
};

// 产生式信息
struct Production {
    int id;
    string lhs; // 左部非终结符
    vector<string> rhs; // 右部符号（终结符或非终结符）
    int rhsLen() const { return rhs.size(); }
};

// [Role A] 负责人：SLR表生成
class SLRGenerator {
private:
    // 核心数据结构
    vector<Production> productions;
    set<string> nonTerminals;
    set<TokenType> terminals;
    
    // 符号到TokenType的映射
    map<string, TokenType> symbolToTokenType;
    
    // FIRST和FOLLOW集
    map<string, set<TokenType>> firstSets;
    map<string, set<TokenType>> followSets;
    
    // 能推导ε的非终结符
    set<string> epsilonDerivingNonTerms;
    
    // LR(0)相关
    struct LR0Item {
        int prodId;      // 产生式ID
        int dotPos;      // 点的位置
        bool operator<(const LR0Item& other) const {
            return tie(prodId, dotPos) < tie(other.prodId, other.dotPos);
        }
    };
    
    struct LR0State {
        int id;
        set<LR0Item> items;
    };
    
    vector<LR0State> states;
    
    // SLR表
    map<int, map<TokenType, Action>> actionTable;
    map<int, map<string, int>> gotoTable;

public:
    // 构建完整SLR分析表
    void build() {
        loadGrammar("../../grammar.txt");
        computeFirstFollow();
        verifyFirstFollow();
        buildLR0();
        buildSLRTable();
        printFirstSets();
        printFollowSets();
        printSLRTable();
    }
    
    // 核心函数
    void loadGrammar(const string& filename);
    void computeFirstFollow();
    void buildLR0();
    void buildSLRTable();
    
    // 新增辅助函数
    void initSymbolMap();
    void computeEpsilonDeriving();
    bool canDeriveEpsilon(const vector<string>& symbols);
    set<TokenType> getFirstOfSymbols(const vector<string>& symbols, int start = 0);
    void verifyFirstFollow();
    
    // LR(0)辅助函数
    set<LR0Item> closure(const set<LR0Item>& items);
    set<LR0Item> goTo(const set<LR0Item>& items, const string& symbol);
    int findState(const set<LR0Item>& items);
    bool isSameItems(const set<LR0Item>& a, const set<LR0Item>& b);
    
    // 提供给Parser的接口
    Action getAction(int state, TokenType type) {
        if (actionTable.count(state) && actionTable[state].count(type))
            return actionTable[state][type];
        return {Action::ERROR, 0};
    }
    
    int getGoto(int state, const string& nonTerminal) {
        if (gotoTable.count(state) && gotoTable[state].count(nonTerminal))
            return gotoTable[state][nonTerminal];
        return -1;
    }
    
    Production getProduction(int id) {
        if (id >= 0 && id < productions.size())
            return productions[id];
        throw runtime_error("Invalid production id");
    }
    
    // 获取内部数据（用于测试）
    const map<string, set<TokenType>>& getFirstSets() const { return firstSets; }
    const map<string, set<TokenType>>& getFollowSets() const { return followSets; }
    const vector<Production>& getProductions() const { return productions; }
    const set<string>& getEpsilonDerivingNonTerms() const { return epsilonDerivingNonTerms; }
    
    // 调试输出
    void printFirstSets();
    void printFollowSets();
    void printSLRTable();
};