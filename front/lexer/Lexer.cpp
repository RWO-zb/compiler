// [Role D] 负责人
#include "Lexer.h"
// 引入原 compiler_ir-master/src/nfa2dfa.cpp 中的逻辑

Token Lexer::getNextToken() {
    // TODO [未完成]: 适配原 LexicalAnaly.cpp 的逻辑
    // 原代码是直接输出到文件，你需要修改它，使其每次调用返回一个 Token 结构体
    // 必须保留原有的 NFA->DFA 自动机逻辑 [任务要求 1]
    
    // 遇到 IDN (标识符) 时：
    // symbolTable->put(idName, nullptr); // 先占位
    
    return currentToken;
}