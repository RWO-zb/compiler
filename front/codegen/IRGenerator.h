#pragma once

#include "../ast/AST.h"
#include "../common/SymbolTable.h"
#include "compiler_ir/include/IRbuilder.h"
#include "compiler_ir/include/Module.h"
#include <map>
#include <string>

// 【新增】定义 ConstVal 结构体，供 evaluateConst 使用
struct ConstVal {
    bool isFloat;
    int i;
    float f;
};

class IRGenerator {
public:
    Module* module;
    IRBuilder* builder;
    Function* currentFunc;

    IRGenerator(Module* m, SymbolTable* st);
    
    // Visitor 方法
    Value* visit(CompUnit* node);
    Value* visit(FuncDef* node);
    Value* visit(BlockStmt* node);
    Value* visit(VarDefStmt* node);
    Value* visit(IfStmt* node);
    Value* visit(ReturnStmt* node);
    Value* visit(BinaryExp* node);
    Value* visit(CallExp* node);
    Value* visit(IdExp* node);
    Value* visit(NumberExp* node);
    Value* visit(FuncFParam* node);

    // 【修改】返回类型改为 ConstVal
    ConstVal evaluateConst(ASTNode* node);

    // 【新增】类型转换辅助函数声明
    Value* typeCast(Value* val, Type* targetType);
};