#include "IRGenerator.h"
#include "../../compiler_ir/include/Constant.h" 
#include "../../compiler_ir/include/BasicBlock.h"
#include "../../compiler_ir/include/Function.h"
#include "../../compiler_ir/include/GlobalVariable.h"
#include "../../compiler_ir/include/Type.h"
#include <iostream>
#include <vector>

IRGenerator::IRGenerator(Module* m, SymbolTable* st) : module(m), currentFunc(nullptr) {
    builder = new IRBuilder(nullptr, module);
}

// 这里的 ASTNode::accept 是为了兼容性，实际上会调用具体的 visit
Value* ASTNode::accept(IRGenerator& gen) { return nullptr; }

Value* IRGenerator::visit(CompUnit* node) {
    for (auto child : node->children) {
        if (child) child->accept(*this);
    }
    return nullptr;
}

// 关键：函数定义，包含参数栈内存分配
Value* IRGenerator::visit(FuncDef* node) {
    std::vector<Type*> paramTypes;
    for (auto p : node->params) {
        // 假设所有参数都是 int32
        paramTypes.push_back(Type::get_int32_type(module));
    }
    
    Type* retType = (node->type == "void") ? Type::get_void_type(module) : Type::get_int32_type(module);
    FunctionType* ft = FunctionType::get(retType, paramTypes);
    Function* f = Function::create(ft, node->name, module);
    currentFunc = f;

    BasicBlock* entry = BasicBlock::create(module, "entry", f);
    builder->set_insert_point(entry);

    // 参数处理：Alloca + Store
    auto args = f->get_args();
    int idx = 0;
    for (auto arg : args) {
        if (idx >= node->params.size()) break;
        FuncFParam* p = node->params[idx];
        
        // 在栈上分配空间
        Value* alloc = builder->create_alloca(Type::get_int32_type(module));
        // 将参数值 store 进去
        builder->create_store(arg, alloc);
        
        // 更新符号表
        valMap[p->name] = alloc; 
        idx++;
    }

    if (node->body) node->body->accept(*this);

    // 补全 Return：防止基本块没有终结指令
    if (!builder->get_insert_block()->get_terminator()) {
        if (node->type == "void") builder->create_void_ret();
        else builder->create_ret(ConstantInt::get(0, module));
    }
    return nullptr;
}

Value* IRGenerator::visit(BlockStmt* node) {
    for (auto stmt : node->stmts) {
        if (stmt) stmt->accept(*this);
    }
    return nullptr;
}

Value* IRGenerator::visit(VarDefStmt* node) {
    Value* alloc = builder->create_alloca(Type::get_int32_type(module));
    valMap[node->name] = alloc;
    if (node->initVal) {
        Value* v = node->initVal->accept(*this);
        builder->create_store(v, alloc);
    }
    return nullptr;
}

Value* IRGenerator::visit(IfStmt* node) {
    Value* cond = node->cond->accept(*this);
    // 如果条件是整数，需要与0比较转为bool (i1)
    if (cond->get_type()->is_integer_type()) 
        cond = builder->create_icmp_ne(cond, ConstantInt::get(0, module));

    Function* f = currentFunc;
    BasicBlock* trueBB = BasicBlock::create(module, "trueBB", f);
    BasicBlock* falseBB = BasicBlock::create(module, "falseBB", f);
    // 如果有 else，我们需要一个汇合块 nextBB；如果没有 else，falseBB 就是汇合块
    BasicBlock* nextBB = node->elseStmt ? BasicBlock::create(module, "nextBB", f) : falseBB;

    // 条件跳转
    if (node->elseStmt)
        builder->create_cond_br(cond, trueBB, falseBB);
    else
        builder->create_cond_br(cond, trueBB, nextBB);

    // True 分支
    builder->set_insert_point(trueBB);
    if (node->thenStmt) node->thenStmt->accept(*this);
    // 只有当当前块没有终结指令（如 return）时才跳转
    if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);

    // Else 分支
    if (node->elseStmt) {
        builder->set_insert_point(falseBB);
        node->elseStmt->accept(*this);
        if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);
        builder->set_insert_point(nextBB); // 继续生成的代码在 nextBB
    } else {
        builder->set_insert_point(nextBB); // 继续生成的代码在 nextBB (即 falseBB)
    }
    return nullptr;
}

Value* IRGenerator::visit(ReturnStmt* node) {
    if (node->retValue) builder->create_ret(node->retValue->accept(*this));
    else builder->create_void_ret();
    return nullptr;
}

Value* IRGenerator::visit(BinaryExp* node) {
    if (node->op == "=") {
        auto id = dynamic_cast<IdExp*>(node->lhs);
        if (valMap.find(id->name) != valMap.end()) {
            Value* ptr = valMap[id->name];
            Value* v = node->rhs->accept(*this);
            builder->create_store(v, ptr);
            return v;
        } else {
            std::cerr << "Assignment to undefined variable: " << id->name << std::endl;
            return ConstantInt::get(0, module);
        }
    }
    
    Value* l = node->lhs->accept(*this);
    Value* r = node->rhs->accept(*this);
    
    // 基本运算
    if (node->op == "+") return builder->create_iadd(l, r);
    if (node->op == "-") return builder->create_isub(l, r);
    if (node->op == "*") return builder->create_imul(l, r);
    if (node->op == "/") return builder->create_isdiv(l, r);
    if (node->op == "%") return builder->create_irem(l, r);
    
    // 比较运算
    Value* cmp = nullptr;
    if (node->op == "<") cmp = builder->create_icmp_lt(l, r);
    if (node->op == ">") cmp = builder->create_icmp_gt(l, r);
    if (node->op == "==") cmp = builder->create_icmp_eq(l, r);
    if (node->op == "!=") cmp = builder->create_icmp_ne(l, r);
    if (node->op == "<=") cmp = builder->create_icmp_le(l, r);
    if (node->op == ">=") cmp = builder->create_icmp_ge(l, r);
    
    if (cmp) {
        // 将 i1 扩展为 i32，因为 C 语言中比较结果是 int
        return builder->create_zext(cmp, Type::get_int32_type(module));
    }

    return ConstantInt::get(0, module);
}

// 关键：函数调用
Value* IRGenerator::visit(CallExp* node) {
    // 修复：Module 类没有 get_function 方法，需要遍历 get_functions() 列表
    Function* f = nullptr;
    for (auto func : module->get_functions()) {
        if (func->get_name() == node->funcName) {
            f = func;
            break;
        }
    }

    if (!f) {
        std::cerr << "Undefined function: " << node->funcName << std::endl;
        return ConstantInt::get(0, module); 
    }
    std::vector<Value*> args;
    for (auto arg : node->args) {
        args.push_back(arg->accept(*this));
    }
    return builder->create_call(f, args);
}

Value* IRGenerator::visit(IdExp* node) {
    if (valMap.find(node->name) != valMap.end()) {
        Value* ptr = valMap[node->name];
        return builder->create_load(ptr);
    }
    std::cerr << "Undefined variable: " << node->name << std::endl;
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(NumberExp* node) {
    return ConstantInt::get(node->val, module);
}

Value* IRGenerator::visit(FuncFParam* node) { 
    return nullptr; 
}