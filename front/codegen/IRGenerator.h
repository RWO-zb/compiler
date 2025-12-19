#pragma once

#include "compiler_ir/include/Module.h"
#include "compiler_ir/include/Function.h"
#include "compiler_ir/include/BasicBlock.h"
#include "compiler_ir/include/Instruction.h"
#include "compiler_ir/include/IRbuilder.h"
#include "front/ast/AST.h"
#include "front/common/SymbolTable.h"
#include <map>
#include <string>

class IRGenerator {
public:
    Module* module;
    IRBuilder* builder;
    SymbolTable* symTable;
    Function* currentFunc;

    // [新增] 用于记录全局变量的编译期数值 (例如 a=7, b=3)
    std::map<std::string, int> globalValues;

    IRGenerator(Module* m, SymbolTable* st);

    // [新增] 编译期计算常量表达式的值
    int evaluateConst(ASTNode* node);

    // Visitor methods
    Value* visit(CompUnit* node);
    Value* visit(FuncDef* node);
    Value* visit(VarDefStmt* node);
    Value* visit(BlockStmt* node);
    Value* visit(IfStmt* node);
    Value* visit(ReturnStmt* node);
    Value* visit(BinaryExp* node);
    Value* visit(CallExp* node);
    Value* visit(NumberExp* node);
    Value* visit(IdExp* node);
    Value* visit(FuncFParam* node);
};