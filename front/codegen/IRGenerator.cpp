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

// 兼容性占位
Value* ASTNode::accept(IRGenerator& gen) { return nullptr; }

Value* IRGenerator::visit(CompUnit* node) {
    for (auto child : node->children) {
        if (child) child->accept(*this);
    }
    return nullptr;
}

Value* IRGenerator::visit(FuncDef* node) {
    std::vector<Type*> paramTypes;
    for (auto p : node->params) {
        paramTypes.push_back(Type::get_int32_type(module));
    }
    
    Type* retType = (node->type == "void") ? Type::get_void_type(module) : Type::get_int32_type(module);
    FunctionType* ft = FunctionType::get(retType, paramTypes);
    Function* f = Function::create(ft, node->name, module);
    currentFunc = f;

    BasicBlock* entry = BasicBlock::create(module, "entry", f);
    builder->set_insert_point(entry);

    auto args = f->get_args();
    int idx = 0;
    for (auto arg : args) {
        if (idx >= node->params.size()) break;
        FuncFParam* p = node->params[idx];
        
        Value* alloc = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(arg, alloc);
        valMap[p->name] = alloc; 
        idx++;
    }

    if (node->body) node->body->accept(*this);

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
    if (cond->get_type()->is_integer_type()) 
        cond = builder->create_icmp_ne(cond, ConstantInt::get(0, module));

    Function* f = currentFunc;
    BasicBlock* trueBB = BasicBlock::create(module, "trueBB", f);
    BasicBlock* falseBB = BasicBlock::create(module, "falseBB", f);
    BasicBlock* nextBB = node->elseStmt ? BasicBlock::create(module, "nextBB", f) : falseBB;

    if (node->elseStmt)
        builder->create_cond_br(cond, trueBB, falseBB);
    else
        builder->create_cond_br(cond, trueBB, nextBB);

    builder->set_insert_point(trueBB);
    if (node->thenStmt) node->thenStmt->accept(*this);
    if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);

    if (node->elseStmt) {
        builder->set_insert_point(falseBB);
        node->elseStmt->accept(*this);
        if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);
        builder->set_insert_point(nextBB); 
    } else {
        builder->set_insert_point(nextBB); 
    }
    return nullptr;
}

Value* IRGenerator::visit(ReturnStmt* node) {
    if (node->retValue) builder->create_ret(node->retValue->accept(*this));
    else builder->create_void_ret();
    return nullptr;
}

Value* IRGenerator::visit(BinaryExp* node) {
    // 赋值特殊处理 ("=" 是 Parser 手动转换的，保持不变)
    if (node->op == "=") {
        auto id = dynamic_cast<IdExp*>(node->lhs);
        if (valMap.find(id->name) != valMap.end()) {
            Value* ptr = valMap[id->name];
            Value* v = node->rhs->accept(*this);
            builder->create_store(v, ptr);
            return v;
        } else {
            return ConstantInt::get(0, module);
        }
    }
    
    Value* l = node->lhs->accept(*this);
    Value* r = node->rhs->accept(*this);
    
    // 【修改】使用文法符号名称进行判断
    if (node->op == "OP_PLUS") return builder->create_iadd(l, r);
    if (node->op == "OP_MINUS") return builder->create_isub(l, r);
    if (node->op == "OP_MUL") return builder->create_imul(l, r);
    if (node->op == "OP_DIV") return builder->create_isdiv(l, r);
    if (node->op == "OP_MOD") return builder->create_irem(l, r);
    
    // 比较运算
    Value* cmp = nullptr;
    if (node->op == "OP_LT") cmp = builder->create_icmp_lt(l, r);
    if (node->op == "OP_GT") cmp = builder->create_icmp_gt(l, r);
    if (node->op == "OP_EQ") cmp = builder->create_icmp_eq(l, r);
    if (node->op == "OP_NEQ") cmp = builder->create_icmp_ne(l, r);
    if (node->op == "OP_LE") cmp = builder->create_icmp_le(l, r);
    if (node->op == "OP_GE") cmp = builder->create_icmp_ge(l, r);
    
    if (cmp) {
        return builder->create_zext(cmp, Type::get_int32_type(module));
    }

    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(CallExp* node) {
    Function* f = nullptr;
    for (auto func : module->get_functions()) {
        if (func->get_name() == node->funcName) {
            f = func;
            break;
        }
    }

    if (!f) {
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
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(NumberExp* node) {
    return ConstantInt::get(node->val, module);
}

Value* IRGenerator::visit(FuncFParam* node) { 
    return nullptr; 
}