#include "IRGenerator.h"
#include "../../compiler_ir/include/Constant.h" 
#include "../../compiler_ir/include/BasicBlock.h"
#include "../../compiler_ir/include/Function.h"

// [Role C] TODO: 实现所有 visit 方法

// 示例：数值
Value* IRGenerator::visit(NumberExp* node) {
    return ConstantInt::get(node->val, module);
}

// 示例：加法
Value* IRGenerator::visit(BinaryExp* node) {
    Value* L = node->lhs->accept(*this);
    Value* R = node->rhs->accept(*this);
    if (node->op == "+") return builder->create_iadd(L, R);
    // ... 其他运算
    return nullptr;
}

// 示例：变量定义
Value* IRGenerator::visit(VarDefStmt* node) {
    Type* type = getLLVMType(node->type);
    auto alloca = builder->create_alloca(type);
    if (node->initVal) {
        Value* init = node->initVal->accept(*this);
        builder->create_store(init, alloca);
    }
    symTable->put(node->name, alloca); // 存入符号表
    return nullptr;
}

// 示例：代码块
Value* IRGenerator::visit(BlockStmt* node) {
    symTable->enterScope();
    for (auto stmt : node->stmts) {
        stmt->accept(*this);
    }
    symTable->exitScope();
    return nullptr;
}

// 必须补充 AST.cpp 中的 accept 实现
Value* BinaryExp::accept(IRGenerator& v) { return v.visit(this); }
Value* NumberExp::accept(IRGenerator& v) { return v.visit(this); }
Value* IdExp::accept(IRGenerator& v) { return v.visit(this); }
Value* BlockStmt::accept(IRGenerator& v) { return v.visit(this); }
Value* IfStmt::accept(IRGenerator& v) { return v.visit(this); }
Value* ReturnStmt::accept(IRGenerator& v) { return v.visit(this); }
Value* VarDefStmt::accept(IRGenerator& v) { return v.visit(this); }
Value* FuncDef::accept(IRGenerator& v) { return v.visit(this); }