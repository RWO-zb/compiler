#pragma once
#include "../ast/AST.h"
#include "../common/SymbolTable.h"
#include "../../compiler_ir/include/IRbuilder.h"
#include "../../compiler_ir/include/Module.h"

// [Role C] 负责人：Visitor 实现
class IRGenerator {
private:
    Module* module;
    IRBuilder* builder;
    SymbolTable* symTable;

    // 辅助：获取 LLVM 类型
    Type* getLLVMType(const std::string& typeName) {
        if (typeName == "int") return Type::get_int32_type(module);
        if (typeName == "void") return Type::get_void_type(module);
        return nullptr;
    }

public:
    IRGenerator(Module* m, SymbolTable* st) : module(m), symTable(st) {
        builder = new IRBuilder(nullptr, m);
    }

    // Visitor 接口
    Value* visit(CompUnit* node);
    Value* visit(FuncDef* node);
    Value* visit(BlockStmt* node);
    Value* visit(VarDefStmt* node);
    Value* visit(ReturnStmt* node);
    Value* visit(IfStmt* node);
    Value* visit(BinaryExp* node);
    Value* visit(NumberExp* node);
    Value* visit(IdExp* node);
};