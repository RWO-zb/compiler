#include "SLRGenerator.h"
#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <utility> // for pair
#include "../common/Token.h"
using namespace std;

// ========== 初始化符号映射 ==========
void SLRGenerator::initSymbolMap() {
    // 关键字映射
    symbolToTokenType["KW_INT"] = KW_INT;
    symbolToTokenType["KW_VOID"] = KW_VOID;
    symbolToTokenType["KW_RETURN"] = KW_RETURN;
    symbolToTokenType["KW_CONST"] = KW_CONST;
    symbolToTokenType["KW_MAIN"] = KW_MAIN;
    symbolToTokenType["KW_FLOAT"] = KW_FLOAT;
    symbolToTokenType["KW_IF"] = KW_IF;
    symbolToTokenType["KW_ELSE"] = KW_ELSE;
    
    // 标识符和常量
    symbolToTokenType["ID"] = ID;
    symbolToTokenType["INT_CONST"] = INT_CONST;
    symbolToTokenType["FLOAT_CONST"] = FLOAT_CONST;
    
    // 运算符
    symbolToTokenType["OP_PLUS"] = OP_PLUS;
    symbolToTokenType["OP_MINUS"] = OP_MINUS;
    symbolToTokenType["OP_MUL"] = OP_MUL;
    symbolToTokenType["OP_DIV"] = OP_DIV;
    symbolToTokenType["OP_MOD"] = OP_MOD;
    symbolToTokenType["OP_ASSIGN"] = OP_ASSIGN;
    symbolToTokenType["OP_NOT"] = OP_NOT;
    symbolToTokenType["OP_EQ"] = OP_EQ;
    symbolToTokenType["OP_NEQ"] = OP_NEQ;
    symbolToTokenType["OP_LT"] = OP_LT;
    symbolToTokenType["OP_GT"] = OP_GT;
    symbolToTokenType["OP_LE"] = OP_LE;
    symbolToTokenType["OP_GE"] = OP_GE;
    symbolToTokenType["OP_AND"] = OP_AND;
    symbolToTokenType["OP_OR"] = OP_OR;
    
    // 界符
    symbolToTokenType["SE_LPAREN"] = SE_LPAREN;
    symbolToTokenType["SE_RPAREN"] = SE_RPAREN;
    symbolToTokenType["SE_LBRACE"] = SE_LBRACE;
    symbolToTokenType["SE_RBRACE"] = SE_RBRACE;
    symbolToTokenType["SE_SEMICOLON"] = SE_SEMICOLON;
    symbolToTokenType["SE_COMMA"] = SE_COMMA;
    
    // 文件结束符
    symbolToTokenType["END_OFF"] = END_OFF;
}

// ========== 加载文法 ==========
void SLRGenerator::loadGrammar(const string& filename) {
    
    // 初始化符号映射
    initSymbolMap();
    
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Cannot open grammar file: " + filename);
    }
    
    string line;
    int prodId = 0;
    
    while (getline(file, line)) {
        // 跳过空行
        if (line.empty()) continue;
        
        // 分割 "->" 左右部分
        size_t arrowPos = line.find("->");
        if (arrowPos == string::npos) continue;
        
        string lhs = line.substr(0, arrowPos);
        // 去除lhs前后的空格
        lhs.erase(0, lhs.find_first_not_of(" \t"));
        lhs.erase(lhs.find_last_not_of(" \t") + 1);
        
        string rhsStr = line.substr(arrowPos + 2);
        
        // 处理"|"分割的多个产生式
        istringstream rhsStream(rhsStr);
        string part;
        vector<string> currentRHS;
        vector<vector<string>> allRHS;
        
        while (rhsStream >> part) {
            if (part == "|") {
                if (!currentRHS.empty()) {
                    allRHS.push_back(currentRHS);
                    currentRHS.clear();
                }
            } else if (part == "ε") {
                // ε产生式：右部为空
                allRHS.push_back({});
                currentRHS.clear();
            } else {
                currentRHS.push_back(part);
            }
        }
        
        // 保存最后一个
        if (!currentRHS.empty()) {
            allRHS.push_back(currentRHS);
        }
        
        // 为每个右部创建产生式
        for (const auto& rhs : allRHS) {
            productions.push_back({prodId++, lhs, rhs});
        }
        
        // 记录非终结符
        nonTerminals.insert(lhs);
    }
    
    file.close();
    
    // 识别终结符
    for (const auto& prod : productions) {
        for (const auto& symbol : prod.rhs) {
            if (nonTerminals.find(symbol) == nonTerminals.end() && 
                symbolToTokenType.find(symbol) != symbolToTokenType.end()) {
                terminals.insert(symbolToTokenType[symbol]);
            }
        }
    }
    
    // 添加增广文法：S' -> Program (作为产生式0)
    productions.insert(productions.begin(), {0, "S'", {"Program"}});
    
    // 重新编号所有产生式
    for (int i = 0; i < productions.size(); i++) {
        productions[i].id = i;
    }
    
    // 计算能推导ε的非终结符
    computeEpsilonDeriving();
}

// ========== 计算能推导ε的非终结符 ==========
void SLRGenerator::computeEpsilonDeriving() {
    epsilonDerivingNonTerms.clear();
    
    // 第一轮：直接的空产生式
    for (const auto& prod : productions) {
        if (prod.rhs.empty()) {
            epsilonDerivingNonTerms.insert(prod.lhs);
        }
    }
    
    // 多轮迭代直到不再变化
    bool changed = true;
    int iteration = 0;
    
    while (changed && iteration < 100) {
        iteration++;
        changed = false;
        
        for (const auto& prod : productions) {
            // 如果已经能推导ε，跳过
            if (epsilonDerivingNonTerms.find(prod.lhs) != epsilonDerivingNonTerms.end()) {
                continue;
            }
            
            // 检查产生式右部：如果所有符号都能推导ε，则左部也能推导ε
            bool allCanDeriveEpsilon = true;
            for (const auto& symbol : prod.rhs) {
                if (nonTerminals.find(symbol) != nonTerminals.end()) {
                    // 是非终结符
                    if (epsilonDerivingNonTerms.find(symbol) == epsilonDerivingNonTerms.end()) {
                        allCanDeriveEpsilon = false;
                        break;
                    }
                } else {
                    // 是终结符或未定义符号，不能推导ε
                    allCanDeriveEpsilon = false;
                    break;
                }
            }
            
            if (allCanDeriveEpsilon) {
                epsilonDerivingNonTerms.insert(prod.lhs);
                changed = true;
            }
        }
    }
}

// ========== 判断符号串是否能推导出ε ==========
bool SLRGenerator::canDeriveEpsilon(const vector<string>& symbols) {
    for (const auto& symbol : symbols) {
        if (nonTerminals.find(symbol) != nonTerminals.end()) {
            // 是非终结符
            if (epsilonDerivingNonTerms.find(symbol) == epsilonDerivingNonTerms.end()) {
                return false;
            }
        } else {
            // 是终结符，不能推导ε
            return false;
        }
    }
    return true;
}

// ========== 获取符号串的FIRST集 ==========
set<TokenType> SLRGenerator::getFirstOfSymbols(const vector<string>& symbols, int start) {
    set<TokenType> result;
    
    for (size_t i = start; i < symbols.size(); i++) {
        const string& symbol = symbols[i];
        
        if (symbolToTokenType.find(symbol) != symbolToTokenType.end()) {
            // 是终结符
            result.insert(symbolToTokenType[symbol]);
            break;
        } else if (nonTerminals.find(symbol) != nonTerminals.end()) {
            // 是非终结符
            // 添加FIRST(symbol)
            if (firstSets.find(symbol) != firstSets.end()) {
                for (TokenType token : firstSets[symbol]) {
                    result.insert(token);
                }
            }
            
            // 如果这个非终结符不能推导ε，停止
            if (epsilonDerivingNonTerms.find(symbol) == epsilonDerivingNonTerms.end()) {
                break;
            }
            
            // 否则继续看下一个符号
        } else {
            // 未知符号，可能是ε或其他
            break;
        }
    }
    
    return result;
}

// ========== 完整的FIRST和FOLLOW集计算 ==========
void SLRGenerator::computeFirstFollow() {
    
    // ========== 计算FIRST集 ==========
    
    // 初始化FIRST集
    for (const auto& nt : nonTerminals) {
        firstSets[nt] = {};
    }
    
    // 多轮迭代直到不再变化
    bool changed = true;
    int firstIteration = 0;
    
    while (changed && firstIteration < 100) {
        firstIteration++;
        changed = false;
        
        for (const auto& prod : productions) {
            const string& A = prod.lhs;
            
            // 处理产生式右部
            if (prod.rhs.empty()) {
                // 空产生式：不添加任何终结符到FIRST集
                continue;
            }
            
            // 对于 A -> X1 X2 ... Xn
            bool allCanDeriveEpsilon = true;
            
            for (size_t i = 0; i < prod.rhs.size(); i++) {
                const string& Xi = prod.rhs[i];
                
                if (symbolToTokenType.find(Xi) != symbolToTokenType.end()) {
                    // Xi是终结符
                    TokenType token = symbolToTokenType[Xi];
                    if (firstSets[A].insert(token).second) {
                        changed = true;
                    }
                    allCanDeriveEpsilon = false;
                    break;
                } else if (nonTerminals.find(Xi) != nonTerminals.end()) {
                    // Xi是非终结符
                    // 将FIRST(Xi)加入FIRST(A)
                    if (firstSets.find(Xi) != firstSets.end()) {
                        for (TokenType token : firstSets[Xi]) {
                            if (firstSets[A].insert(token).second) {
                                changed = true;
                            }
                        }
                    }
                    
                    // 如果Xi不能推导ε，停止
                    if (epsilonDerivingNonTerms.find(Xi) == epsilonDerivingNonTerms.end()) {
                        allCanDeriveEpsilon = false;
                        break;
                    }
                    // 否则继续检查下一个符号
                } else {
                    // 未知符号
                    allCanDeriveEpsilon = false;
                    break;
                }
            }
        }
    }
    
    // ========== 计算FOLLOW集 ==========
    
    // 初始化FOLLOW集
    for (const auto& nt : nonTerminals) {
        followSets[nt] = {};
    }
    
    // FOLLOW(Program) 包含 $
    followSets["Program"].insert(END_OFF);
    
    // 多轮迭代计算FOLLOW集
    changed = true;
    int followIteration = 0;
    
    while (changed && followIteration < 100) {
        followIteration++;
        changed = false;
        
        for (const auto& prod : productions) {
            const string& A = prod.lhs;
            
            for (size_t i = 0; i < prod.rhs.size(); i++) {
                const string& B = prod.rhs[i];
                
                // 如果B是非终结符
                if (nonTerminals.find(B) != nonTerminals.end()) {
                    // 情况1：B后面有符号
                    if (i + 1 < prod.rhs.size()) {
                        // 获取B后面符号串的FIRST集
                        vector<string> beta(prod.rhs.begin() + i + 1, prod.rhs.end());
                        set<TokenType> firstOfBeta = getFirstOfSymbols(beta);
                        
                        // 将FIRST(β)加入FOLLOW(B)
                        for (TokenType token : firstOfBeta) {
                            if (followSets[B].insert(token).second) {
                                changed = true;
                            }
                        }
                    }
                    
                    // 情况2：B在末尾，或者B后面的符号串能推导ε
                    bool restCanDeriveEpsilon = true;
                    for (size_t j = i + 1; j < prod.rhs.size(); j++) {
                        const string& symbol = prod.rhs[j];
                        if (nonTerminals.find(symbol) != nonTerminals.end()) {
                            if (epsilonDerivingNonTerms.find(symbol) == epsilonDerivingNonTerms.end()) {
                                restCanDeriveEpsilon = false;
                                break;
                            }
                        } else {
                            // 有终结符，肯定不能推导ε
                            restCanDeriveEpsilon = false;
                            break;
                        }
                    }
                    
                    if (i + 1 >= prod.rhs.size() || restCanDeriveEpsilon) {
                        // 将FOLLOW(A)加入FOLLOW(B)
                        if (followSets.find(A) != followSets.end()) {
                            for (TokenType token : followSets[A]) {
                                if (followSets[B].insert(token).second) {
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// ========== 验证FIRST/FOLLOW集正确性 ==========
void SLRGenerator::verifyFirstFollow() {
    // [修改] 整个函数内容注释掉或仅注释 cout
    /*
    cout << "=== Verifying FIRST and FOLLOW sets ===\n";
    
    // 检查一些关键非终结符
    vector<string> keyNonTerms = {"exp", "stmt", "decl", "btype", "compUnit", "Program"};
    
    for (const auto& nt : keyNonTerms) {
        if (firstSets.find(nt) != firstSets.end()) {
            cout << "FIRST(" << nt << ") = { ";
            for (TokenType token : firstSets[nt]) {
                cout << static_cast<int>(token) << " ";
            }
            cout << "}\n";
        } else {
            cout << "FIRST(" << nt << ") not found!\n";
        }
    }
    
    cout << "\n";
    
    for (const auto& nt : keyNonTerms) {
        if (followSets.find(nt) != followSets.end()) {
            cout << "FOLLOW(" << nt << ") = { ";
            for (TokenType token : followSets[nt]) {
                cout << static_cast<int>(token) << " ";
            }
            cout << "}\n";
        } else {
            cout << "FOLLOW(" << nt << ") not found!\n";
        }
    }
    
    // 验证一些基本属性
    cout << "\nBasic verification:\n";
    
    // 1. btype的FIRST应该包含KW_INT和KW_FLOAT
    if (firstSets.find("btype") != firstSets.end()) {
        const auto& btypeFirst = firstSets["btype"];
        if (btypeFirst.find(KW_INT) != btypeFirst.end() && 
            btypeFirst.find(KW_FLOAT) != btypeFirst.end()) {
            cout << "✓ btype FIRST set contains KW_INT and KW_FLOAT\n";
        } else {
            cout << "✗ btype FIRST set missing KW_INT or KW_FLOAT\n";
        }
    }
    
    // 2. Program的FOLLOW应该包含END_OFF
    if (followSets.find("Program") != followSets.end()) {
        const auto& programFollow = followSets["Program"];
        if (programFollow.find(END_OFF) != programFollow.end()) {
            cout << "✓ Program FOLLOW set contains END_OFF\n";
        } else {
            cout << "✗ Program FOLLOW set missing END_OFF\n";
        }
    }
    
    cout << "\n";
    */
}

// ========== 计算闭包 ==========
set<SLRGenerator::LR0Item> SLRGenerator::closure(const set<LR0Item>& items) {
    set<LR0Item> closureSet = items;
    bool changed = true;
    
    // 预计算非终结符到产生式的映射，提高效率
    static map<string, vector<int>> ntToProds;
    if (ntToProds.empty()) {
        for (int i = 0; i < productions.size(); i++) {
            ntToProds[productions[i].lhs].push_back(i);
        }
    }
    
    while (changed) {
        changed = false;
        vector<LR0Item> itemsToAdd;
        
        for (const auto& item : closureSet) {
            const Production& prod = productions[item.prodId];
            
            // 如果点在末尾，跳过
            if (item.dotPos >= prod.rhs.size()) continue;
            
            string nextSymbol = prod.rhs[item.dotPos];
            
            // 如果下一个符号是非终结符，添加它的所有产生式
            if (nonTerminals.find(nextSymbol) != nonTerminals.end()) {
                for (int prodId : ntToProds[nextSymbol]) {
                    LR0Item newItem{prodId, 0};
                    if (closureSet.find(newItem) == closureSet.end()) {
                        itemsToAdd.push_back(newItem);
                    }
                }
            }
        }
        
        if (!itemsToAdd.empty()) {
            changed = true;
            for (const auto& item : itemsToAdd) {
                closureSet.insert(item);
            }
        }
    }
    
    return closureSet;
}

// ========== 计算GOTO ==========
set<SLRGenerator::LR0Item> SLRGenerator::goTo(const set<LR0Item>& items, const string& symbol) {
    set<LR0Item> gotoSet;
    
    for (const auto& item : items) {
        const Production& prod = productions[item.prodId];
        
        // 如果点在末尾，跳过
        if (item.dotPos >= prod.rhs.size()) continue;
        
        // 如果下一个符号是我们要转移的符号
        if (prod.rhs[item.dotPos] == symbol) {
            gotoSet.insert({item.prodId, item.dotPos + 1});
        }
    }
    
    return closure(gotoSet);
}

// ========== 查找状态 ==========
int SLRGenerator::findState(const set<LR0Item>& items) {
    for (int i = 0; i < states.size(); i++) {
        if (isSameItems(states[i].items, items)) {
            return i;
        }
    }
    return -1;
}

// ========== 比较项目集 ==========
bool SLRGenerator::isSameItems(const set<LR0Item>& a, const set<LR0Item>& b) {
    if (a.size() != b.size()) return false;
    
    auto itA = a.begin();
    auto itB = b.begin();
    
    while (itA != a.end() && itB != b.end()) {
        if (itA->prodId != itB->prodId || itA->dotPos != itB->dotPos) {
            return false;
        }
        ++itA;
        ++itB;
    }
    
    return true;
}

// ========== 构建LR(0)项目集规范族 ==========
void SLRGenerator::buildLR0() {
    
    // 创建初始状态：S' -> ·Program
    set<LR0Item> initialItems;
    initialItems.insert({0, 0}); // 产生式0: S' -> Program，点在开头
    
    set<LR0Item> initialClosure = closure(initialItems);
    
    // 创建初始状态
    LR0State initialState;
    initialState.id = 0;
    initialState.items = initialClosure;
    states.push_back(initialState);
    
    // 收集所有可能出现在产生式右部的符号
    set<string> allSymbols;
    for (const auto& prod : productions) {
        for (const auto& symbol : prod.rhs) {
            allSymbols.insert(symbol);
        }
    }
    // 添加所有非终结符
    allSymbols.insert(nonTerminals.begin(), nonTerminals.end());
    
    // 工作列表：(stateId, symbol)
    vector<pair<int, string>> workList;
    
    // 为初始状态添加所有符号
    for (const auto& symbol : allSymbols) {
        workList.push_back({0, symbol});
    }
    
    // 处理工作列表
    int statesCreated = 1;
    
    while (!workList.empty()) {
        int stateId = workList.back().first;
        string symbol = workList.back().second;
        workList.pop_back();
        
        const LR0State& currentState = states[stateId];
        set<LR0Item> gotoItems = goTo(currentState.items, symbol);
        
        if (!gotoItems.empty()) {
            int existingState = findState(gotoItems);
            
            if (existingState == -1) {
                // 创建新状态
                LR0State newState;
                newState.id = states.size();
                newState.items = gotoItems;
                states.push_back(newState);
                statesCreated++;
                
                // 为新状态添加所有符号到工作列表
                for (const auto& sym : allSymbols) {
                    workList.push_back({newState.id, sym});
                }
                
                existingState = newState.id;
            }
        }
    }
    
}

// ========== 构建SLR分析表 ==========
void SLRGenerator::buildSLRTable() {
    
    // 清空表
    actionTable.clear();
    gotoTable.clear();
    
    // 为每个状态初始化表项
    for (int i = 0; i < states.size(); i++) {
        actionTable[i] = {};
        gotoTable[i] = {};
    }
    
    // 临时存储goto结果，避免重复计算
    map<pair<int, string>, int> gotoCache;
    
    // 先计算所有非终结符的goto
    for (int i = 0; i < states.size(); i++) {
        for (const auto& symbol : nonTerminals) {
            set<LR0Item> gotoResult = goTo(states[i].items, symbol);
            if (!gotoResult.empty()) {
                int j = findState(gotoResult);
                if (j != -1) {
                    gotoCache[{i, symbol}] = j;
                    gotoTable[i][symbol] = j;
                }
            }
        }
    }
    
    // 构建分析表
    int shiftCount = 0, reduceCount = 0, acceptCount = 0, errorCount = 0;
    
    for (int i = 0; i < states.size(); i++) {
        const LR0State& state = states[i];
        
        // 检查每个项目
        for (const auto& item : state.items) {
            const Production& prod = productions[item.prodId];
            
            if (item.dotPos < prod.rhs.size()) {
                // 点在中间：移进
                string nextSymbol = prod.rhs[item.dotPos];
                
                // 检查goto缓存或计算
                int j = -1;
                auto cacheKey = make_pair(i, nextSymbol);
                
                if (gotoCache.find(cacheKey) != gotoCache.end()) {
                    j = gotoCache[cacheKey];
                } else {
                    set<LR0Item> gotoResult = goTo(state.items, nextSymbol);
                    j = findState(gotoResult);
                    if (j != -1) {
                        gotoCache[cacheKey] = j;
                    }
                }
                
                if (j != -1 && symbolToTokenType.find(nextSymbol) != symbolToTokenType.end()) {
                    // 下一个是终结符：移进动作
                    TokenType token = symbolToTokenType[nextSymbol];
                    
                    // 检查冲突
                    if (actionTable[i].find(token) != actionTable[i].end()) {
                        Action existing = actionTable[i][token];
                        if (existing.type == Action::REDUCE) {
                            /*
                            cout << "Shift-Reduce conflict at state " << i 
                                      << " on token " << static_cast<int>(token) 
                                      << " (Shift preferred)\n";
                            */
                        }
                    }
                    
                    actionTable[i][token] = {Action::SHIFT, j};
                    shiftCount++;
                }
            } else {
                // 点在末尾：规约或接受
                if (prod.id == 0) { // S' -> Program·
                    // 接受动作
                    actionTable[i][END_OFF] = {Action::ACCEPT, 0};
                    acceptCount++;
                } else {
                    // 规约动作：对FOLLOW(prod.lhs)中的每个终结符
                    if (followSets.find(prod.lhs) != followSets.end()) {
                        for (TokenType token : followSets[prod.lhs]) {
                            // 检查冲突
                            if (actionTable[i].find(token) != actionTable[i].end()) {
                                Action existing = actionTable[i][token];
                                if (existing.type == Action::SHIFT) {
                                    /*
                                    cout << "Shift-Reduce conflict at state " << i 
                                              << " on token " << static_cast<int>(token) 
                                              << " (Reduction by production " << prod.id << ")\n";
                                    */
                                    continue; // SLR冲突：优先移进
                                } else if (existing.type == Action::REDUCE) {
                                    /*
                                    cout << "Reduce-Reduce conflict at state " << i 
                                              << " on token " << static_cast<int>(token) 
                                              << " between productions " << existing.target 
                                              << " and " << prod.id << "\n";
                                    */
                                }
                            }
                            
                            actionTable[i][token] = {Action::REDUCE, prod.id};
                            reduceCount++;
                        }
                    }
                }
            }
        }
    }
    
    // 填充默认错误动作
    for (int i = 0; i < states.size(); i++) {
        for (TokenType token : terminals) {
            if (actionTable[i].find(token) == actionTable[i].end()) {
                actionTable[i][token] = {Action::ERROR, 0};
                errorCount++;
            }
        }
    }
    
}

// ========== 调试输出函数  ==========
void SLRGenerator::printFirstSets() {
    /*
    cout << "=== FIRST Sets ===\n";
    for (const auto& pair : firstSets) {
        cout << "FIRST(" << pair.first << ") = { ";
        for (TokenType token : pair.second) {
            cout << static_cast<int>(token) << " ";
        }
        cout << "}\n";
    }
    cout << "\n";
    */
}

void SLRGenerator::printFollowSets() {
    /*
    cout << "=== FOLLOW Sets ===\n";
    for (const auto& pair : followSets) {
        cout << "FOLLOW(" << pair.first << ") = { ";
        for (TokenType token : pair.second) {
            cout << static_cast<int>(token) << " ";
        }
        cout << "}\n";
    }
    cout << "\n";
    */
}

void SLRGenerator::printSLRTable() {
    /*
    cout << "=== SLR Parsing Table (Summary) ===\n";
    ... 
    */
}