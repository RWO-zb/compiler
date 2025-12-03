#pragma once
#include "SLRGenerator.h"
#include "../lexer/Lexer.h"
#include "../ast/AST.h"
#include <stack>
#include <iostream>

// [Role B] 负责人
class Parser {
private:
    Lexer& lexer;
    SLRGenerator& slr;

    std::stack<int> stateStack;       // SLR 状态栈
    std::stack<ASTNode*> nodeStack;   // AST 节点栈

    // 将 token 转换成一个最基础的 ASTNode（终结符）
    ASTNode* makeLeaf(const Token& tok);

    // 根据产生式构造 AST
    ASTNode* buildAST(const Production& prod,
                      std::vector<ASTNode*>& children);

public:
    Parser(Lexer& l, SLRGenerator& s) : lexer(l), slr(s) {}

    ASTNode* parse(); // 主入口
};