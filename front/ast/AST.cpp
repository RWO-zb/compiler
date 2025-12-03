#include "AST.h"
#include "../codegen/IRGenerator.h"

// ASTNode 基类的默认实现（通常不会被调用，因为是纯虚函数，但为了安全保留）
// 注意：如果 ASTNode 中是纯虚函数 (=0)，则不需要这行，但在某些实现中为了兼容性保留
// 这里我们假设 ASTNode 的 accept 是纯虚的，具体实现在子类

// 编译单元
Value* CompUnit::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 函数参数（新增）
Value* FuncFParam::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 代码块
Value* BlockStmt::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 函数定义
Value* FuncDef::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 变量定义
Value* VarDefStmt::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// If 语句
Value* IfStmt::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// Return 语句
Value* ReturnStmt::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 二元表达式
Value* BinaryExp::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 函数调用（新增 - 修复 LNK2001 错误的关键）
Value* CallExp::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 数字常量
Value* NumberExp::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}

// 标识符
Value* IdExp::accept(IRGenerator& gen) { 
    return gen.visit(this); 
}