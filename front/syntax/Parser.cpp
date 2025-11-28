#include "Parser.h"
#include <iostream>

ASTNode* Parser::parse() {
    stateStack.push(0); // 初始状态

    while (true) {
        Token token = lexer.peek();
        Action action = slr.getAction(stateStack.top(), token.type);

        if (action.type == Action::SHIFT) {
            stateStack.push(action.target);
            lexer.next(); // 消耗 Token
            // [Role B] TODO: 将 Token 包装成终结符节点压入 nodeStack (如果需要)
            // nodeStack.push(nullptr); 
        } 
        else if (action.type == Action::REDUCE) {
            SLRGenerator::Production prod = slr.getProduction(action.target);
            
            // [Role B] TODO: 核心构建逻辑
            // 1. 弹出 prod.rhsLen 个 ASTNode*
            // 2. new 一个新的父节点 (如 BinaryExp), 将弹出的子节点挂载上去
            // 3. 将父节点 push 到 nodeStack
            
            // 示例: A -> B + C
            if (prod.lhs == "Exp" /* && prod is B+C */) {
                // ASTNode* right = nodeStack.top(); nodeStack.pop();
                // ASTNode* left = nodeStack.top(); nodeStack.pop();
                // nodeStack.push(new BinaryExp("+", left, right));
            }
            
            // 状态转移
            int currentState = stateStack.top(); // 注意：这里需要先弹出 rhsLen 个状态
            // stateStack.pop() ... (弹出 len 个)
            // int nextState = slr.getGoto(stateStack.top(), prod.lhs);
            // stateStack.push(nextState);
            
            // 打印规约 (作业要求)
            std::cout << "Reduce: " << prod.id << std::endl;
        } 
        else if (action.type == Action::ACCEPT) {
            return nodeStack.top();
        } 
        else {
            std::cerr << "Syntax Error at line " << token.line << std::endl;
            return nullptr;
        }
    }
}