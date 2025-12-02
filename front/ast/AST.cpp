#include "AST.h"
#include "../codegen/IRGenerator.h"

/**
 * 各个AST节点的accept方法实现
 * 这些方法需要调用IRGenerator对应的visit方法
 */

Value* BinaryExp::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* NumberExp::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* IdExp::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* BlockStmt::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* IfStmt::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* ReturnStmt::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* VarDefStmt::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* FuncDef::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}

Value* CompUnit::accept(IRGenerator& visitor) {
    return visitor.visit(this);
}