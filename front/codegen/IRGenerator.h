#pragma once
#include "../ast/AST.h"
#include "../../compiler_ir/include/IRbuilder.h"
#include "../../compiler_ir/include/Module.h"
#include "../../compiler_ir/include/Function.h"
#include "../../compiler_ir/include/BasicBlock.h"
#include "../../compiler_ir/include/Instruction.h"
#include "../../compiler_ir/include/Value.h"
#include "../common/SymbolTable.h"
#include <map>
#include <string>

class IRGenerator {
private:
    Module* module;
    IRBuilder* builder;
    
    // AST 变量名 -> IR Value (alloca指令的地址) 的映射表
    std::map<std::string, Value*> valMap;

    // 当前正在处理的函数
    Function* currentFunc;

public:
    // 构造函数声明
    IRGenerator(Module* m, SymbolTable* st);

    Module* getModule() { return module; }

    // Visitor pattern: 接收 AST 节点指针，返回 IR Value 指针
    Value* visit(CompUnit* node);
    Value* visit(FuncDef* node);
    Value* visit(FuncFParam* node); // 【新增】
    Value* visit(BlockStmt* node);
    Value* visit(VarDefStmt* node);
    Value* visit(IfStmt* node);
    Value* visit(ReturnStmt* node);
    Value* visit(BinaryExp* node);
    Value* visit(CallExp* node);    // 【新增】
    Value* visit(NumberExp* node);
    Value* visit(IdExp* node);
};