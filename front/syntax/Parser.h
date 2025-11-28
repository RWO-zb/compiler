#pragma once
#include "SLRGenerator.h"
#include "../lexer/Lexer.h"
#include "../ast/AST.h"
#include <stack>

// [Role B] 负责人
class Parser {
private:
    Lexer& lexer;
    SLRGenerator& slr;
    std::stack<int> stateStack;
    std::stack<ASTNode*> nodeStack; // 节点栈

public:
    Parser(Lexer& l, SLRGenerator& s) : lexer(l), slr(s) {}

    ASTNode* parse(); // 入口
};