#include "IRGenerator.h" // 确保包含了更新后的头文件
#include "compiler_ir/include/Constant.h" 
#include "compiler_ir/include/BasicBlock.h"
#include "compiler_ir/include/Function.h"
#include "compiler_ir/include/GlobalVariable.h"
#include "compiler_ir/include/Type.h"
#include <iostream>
#include <vector>
#include <map>

// ========== 辅助类：作用域管理 ==========
class ScopeManager {
public:
    std::vector<std::map<std::string, Value*>> scopes;
    
    ScopeManager() { enter(); }
    void enter() { scopes.emplace_back(); }
    void exit() { if (!scopes.empty()) scopes.pop_back(); }
    bool declare(const std::string& name, Value* val) {
        if (scopes.empty()) return false;
        if (scopes.back().find(name) != scopes.back().end()) return false;
        scopes.back()[name] = val;
        return true;
    }
    Value* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) return it->at(name);
        }
        return nullptr;
    }
    bool isGlobal() const { return scopes.size() == 1; }
};

static ScopeManager scopeMgr;

// 全局常量表
static std::map<std::string, ConstVal> globalConstValues;

// ========== IRGenerator 实现 ==========

IRGenerator::IRGenerator(Module* m, SymbolTable* st) : module(m), currentFunc(nullptr) {
    builder = new IRBuilder(nullptr, module);
    scopeMgr = ScopeManager(); // 重置作用域
    globalConstValues.clear(); // 清空全局常量表
}

// 【修改】作为成员函数实现 typeCast
Value* IRGenerator::typeCast(Value* val, Type* targetType) {
    Type* srcType = val->get_type();
    if (Type::is_eq_type(srcType, targetType)) return val;

    // int32 -> float
    if (srcType->is_int32_type() && targetType->is_float_type()) {
        return builder->create_sitofp(val, targetType);
    }
    // float -> int32
    if (srcType->is_float_type() && targetType->is_int32_type()) {
        return builder->create_fptosi(val, targetType);
    }
    // int1 (bool) -> int32
    if (srcType->is_int1_type() && targetType->is_int32_type()) {
        return builder->create_zext(val, targetType);
    }
    
    // int32 -> int1 (bool)
    if (srcType->is_int32_type() && targetType->is_int1_type()) {
        return builder->create_icmp_ne(val, ConstantInt::get(0, module));
    }
    // float -> int1 (bool)
    if (srcType->is_float_type() && targetType->is_int1_type()) {
        return builder->create_fcmp_ne(val, ConstantFloat::get(0.0, module));
    }

    return val;
}

// 【修改】实现 evaluateConst，返回类型为 ConstVal
ConstVal IRGenerator::evaluateConst(ASTNode* node) {
    if (auto num = dynamic_cast<NumberExp*>(node)) {
        if (num->isFloat) return {true, 0, num->floatVal};
        return {false, num->intVal, 0.0f};
    }
    else if (auto id = dynamic_cast<IdExp*>(node)) {
        if (globalConstValues.count(id->name)) {
            return globalConstValues[id->name];
        }
        std::cerr << "Error: Global initializer refers to unknown or non-const variable: " << id->name << std::endl;
        return {false, 0, 0.0f};
    }
    else if (auto bin = dynamic_cast<BinaryExp*>(node)) {
        ConstVal l = evaluateConst(bin->lhs);
        ConstVal r = evaluateConst(bin->rhs);
        
        bool resIsFloat = l.isFloat || r.isFloat;
        float lf = l.isFloat ? l.f : (float)l.i;
        float rf = r.isFloat ? r.f : (float)r.i;
        int li = l.isFloat ? (int)l.f : l.i;
        int ri = r.isFloat ? (int)r.f : r.i;
        
        std::string op = bin->op;
        
        if (resIsFloat) {
            if (op == "+" || op == "OP_PLUS") return {true, 0, lf + rf};
            if (op == "-" || op == "OP_MINUS") return {true, 0, lf - rf};
            if (op == "*" || op == "OP_MUL") return {true, 0, lf * rf};
            if (op == "/" || op == "OP_DIV") return {true, 0, (rf != 0 ? lf / rf : 0)};
            // 比较运算返回整数
            if (op == ">" || op == "OP_GT") return {false, lf > rf, 0.0f};
            if (op == "<" || op == "OP_LT") return {false, lf < rf, 0.0f};
            // ... 其他比较同理
        } 
        else {
            if (op == "+" || op == "OP_PLUS") return {false, li + ri, 0.0f};
            if (op == "-" || op == "OP_MINUS") return {false, li - ri, 0.0f};
            if (op == "*" || op == "OP_MUL") return {false, li * ri, 0.0f};
            if (op == "/" || op == "OP_DIV") return {false, (ri != 0 ? li / ri : 0), 0.0f};
            if (op == "%" || op == "OP_MOD") return {false, (ri != 0 ? li % ri : 0), 0.0f};
        }
    }
    return {false, 0, 0.0f};
}

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
        if (p->type == "float") paramTypes.push_back(Type::get_float_type(module));
        else paramTypes.push_back(Type::get_int32_type(module));
    }
    
    Type* retType = Type::get_void_type(module);
    if (node->type == "int") retType = Type::get_int32_type(module);
    else if (node->type == "float") retType = Type::get_float_type(module);
    
    FunctionType* ft = FunctionType::get(retType, paramTypes);
    Function* f = Function::create(ft, node->name, module);
    currentFunc = f;
    
    BasicBlock* entry = BasicBlock::create(module, "entry", f);
    builder->set_insert_point(entry);

    scopeMgr.enter();

    auto args = f->get_args();
    int idx = 0;
    for (auto arg : args) {
        if (idx >= node->params.size()) break;
        FuncFParam* p = node->params[idx];
        Type* argType = arg->get_type();
        
        Value* alloc = builder->create_alloca(argType);
        builder->create_store(arg, alloc);
        scopeMgr.declare(p->name, alloc);
        idx++;
    }

    if (node->body) node->body->accept(*this);

    if (!builder->get_insert_block()->get_terminator()) {
        if (retType->is_void_type()) builder->create_void_ret();
        else if (retType->is_float_type()) builder->create_ret(ConstantFloat::get(0.0, module));
        else builder->create_ret(ConstantInt::get(0, module));
    }
    
    scopeMgr.exit();
    currentFunc = nullptr;
    return nullptr;
}

Value* IRGenerator::visit(BlockStmt* node) {
    scopeMgr.enter();
    for (auto stmt : node->stmts) {
        if (builder->get_insert_block()->get_terminator()) {
            break; // 停止生成后续死代码
        }
        if (stmt) stmt->accept(*this);
    }
    scopeMgr.exit();
    return nullptr;
}

Value* IRGenerator::visit(VarDefStmt* node) {
    // 【修改】修复三元运算符类型不匹配 C2446
    Type* varType = nullptr;
    if (node->type == "float") 
        varType = Type::get_float_type(module);
    else 
        varType = Type::get_int32_type(module);

    if (scopeMgr.isGlobal()) {
        Constant* initConst = nullptr;
        ConstVal val = {false, 0, 0.0f};

        if (node->initVal) {
            val = evaluateConst(node->initVal);
            if (varType->is_float_type()) {
                float f = val.isFloat ? val.f : (float)val.i;
                initConst = ConstantFloat::get(f, module);
                globalConstValues[node->name] = {true, 0, f};
            } else {
                int i = val.isFloat ? (int)val.f : val.i;
                initConst = ConstantInt::get(i, module);
                globalConstValues[node->name] = {false, i, 0.0f};
            }
        } else {
            initConst = ConstantZero::get(varType, module);
            globalConstValues[node->name] = {varType->is_float_type(), 0, 0.0f};
        }

        GlobalVariable* gVar = GlobalVariable::create(node->name, module, varType, false, initConst);
        scopeMgr.declare(node->name, gVar);

    } else {
        Value* alloc = builder->create_alloca(varType);
        scopeMgr.declare(node->name, alloc);
        
        if (node->initVal) {
            Value* v = node->initVal->accept(*this);
            v = typeCast(v, varType);
            builder->create_store(v, alloc);
        }
    }
    return nullptr;
}

Value* IRGenerator::visit(IfStmt* node) {
    Value* cond = node->cond->accept(*this);
    cond = typeCast(cond, Type::get_int1_type(module));

    Function* f = currentFunc;
    BasicBlock* trueBB = BasicBlock::create(module, "if_true", f);
    BasicBlock* falseBB = BasicBlock::create(module, "if_false", f);
    BasicBlock* nextBB = BasicBlock::create(module, "if_next", f);
    
    BasicBlock* falseTarget = node->elseStmt ? falseBB : nextBB;

    builder->create_cond_br(cond, trueBB, falseTarget);

    builder->set_insert_point(trueBB);
    if (node->thenStmt) node->thenStmt->accept(*this);
    if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);

    if (node->elseStmt) {
        builder->set_insert_point(falseBB);
        node->elseStmt->accept(*this);
        if (!builder->get_insert_block()->get_terminator()) builder->create_br(nextBB);
    }
    
    builder->set_insert_point(nextBB); 
    return nullptr;
}

Value* IRGenerator::visit(ReturnStmt* node) {
    FunctionType* funcType = currentFunc->get_function_type();
    Type* retType = funcType->get_return_type();

    if (node->retValue) {
        Value* v = node->retValue->accept(*this);
        v = typeCast(v, retType);
        builder->create_ret(v);
    } else {
        builder->create_void_ret();
    }
    return nullptr;
}

Value* IRGenerator::visit(BinaryExp* node) {
    std::string op = node->op;

    if (op == "=" || op == "OP_ASSIGN") {
        auto id = dynamic_cast<IdExp*>(node->lhs);
        Value* ptr = scopeMgr.lookup(id->name);
        if (ptr) {
            Value* v = node->rhs->accept(*this);
            Type* targetType = ptr->get_type()->get_pointer_element_type();
            v = typeCast(v, targetType);
            builder->create_store(v, ptr);
            return v;
        }
        return ConstantInt::get(0, module);
    }
    
    if (op == "&&" || op == "OP_AND") {
        Value* l = node->lhs->accept(*this);
        l = typeCast(l, Type::get_int1_type(module));

        BasicBlock* rhsBB = BasicBlock::create(module, "and_rhs", currentFunc);
        BasicBlock* endBB = BasicBlock::create(module, "and_end", currentFunc);
        
        Value* resVar = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(ConstantInt::get(0, module), resVar); 
        
        builder->create_cond_br(l, rhsBB, endBB);
        
        builder->set_insert_point(rhsBB);
        Value* r = node->rhs->accept(*this);
        r = typeCast(r, Type::get_int1_type(module)); 
        builder->create_store(builder->create_zext(r, Type::get_int32_type(module)), resVar);
        builder->create_br(endBB);
        
        builder->set_insert_point(endBB);
        return builder->create_load(resVar);
    }
    if (op == "||" || op == "OP_OR") {
        Value* l = node->lhs->accept(*this);
        l = typeCast(l, Type::get_int1_type(module));

        BasicBlock* rhsBB = BasicBlock::create(module, "or_rhs", currentFunc);
        BasicBlock* endBB = BasicBlock::create(module, "or_end", currentFunc);
        
        Value* resVar = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(ConstantInt::get(1, module), resVar); 
        
        builder->create_cond_br(l, endBB, rhsBB);
        
        builder->set_insert_point(rhsBB);
        Value* r = node->rhs->accept(*this);
        r = typeCast(r, Type::get_int1_type(module));
        builder->create_store(builder->create_zext(r, Type::get_int32_type(module)), resVar);
        builder->create_br(endBB);
        
        builder->set_insert_point(endBB);
        return builder->create_load(resVar);
    }

    Value* l = node->lhs->accept(*this);
    Value* r = node->rhs->accept(*this);
    
    bool isFloatOp = l->get_type()->is_float_type() || r->get_type()->is_float_type();
    
    if (isFloatOp) {
        l = typeCast(l, Type::get_float_type(module));
        r = typeCast(r, Type::get_float_type(module));
    }

    if (isFloatOp) {
        if (op == "+" || op == "OP_PLUS") return builder->create_fadd(l, r);
        if (op == "-" || op == "OP_MINUS") return builder->create_fsub(l, r);
        if (op == "*" || op == "OP_MUL") return builder->create_fmul(l, r);
        if (op == "/" || op == "OP_DIV") return builder->create_fdiv(l, r);
        
        Value* cmp = nullptr;
        if (op == "<"  || op == "OP_LT") cmp = builder->create_fcmp_lt(l, r);
        if (op == ">"  || op == "OP_GT") cmp = builder->create_fcmp_gt(l, r);
        if (op == "==" || op == "OP_EQ") cmp = builder->create_fcmp_eq(l, r);
        if (op == "!=" || op == "OP_NEQ") cmp = builder->create_fcmp_ne(l, r);
        if (op == "<=" || op == "OP_LE") cmp = builder->create_fcmp_le(l, r);
        if (op == ">=" || op == "OP_GE") cmp = builder->create_fcmp_ge(l, r);
        
        if (cmp) return builder->create_zext(cmp, Type::get_int32_type(module));
    } else {
        if (op == "+" || op == "OP_PLUS") return builder->create_iadd(l, r);
        if (op == "-" || op == "OP_MINUS") return builder->create_isub(l, r);
        if (op == "*" || op == "OP_MUL") return builder->create_imul(l, r);
        if (op == "/" || op == "OP_DIV") return builder->create_isdiv(l, r);
        if (op == "%" || op == "OP_MOD") return builder->create_irem(l, r);
        
        Value* cmp = nullptr;
        if (op == "<"  || op == "OP_LT") cmp = builder->create_icmp_lt(l, r);
        if (op == ">"  || op == "OP_GT") cmp = builder->create_icmp_gt(l, r);
        if (op == "==" || op == "OP_EQ") cmp = builder->create_icmp_eq(l, r);
        if (op == "!=" || op == "OP_NEQ") cmp = builder->create_icmp_ne(l, r);
        if (op == "<=" || op == "OP_LE") cmp = builder->create_icmp_le(l, r);
        if (op == ">=" || op == "OP_GE") cmp = builder->create_icmp_ge(l, r);
        
        if (cmp) return builder->create_zext(cmp, Type::get_int32_type(module));
    }
    
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(CallExp* node) {
    Function* f = nullptr;
    for (auto func : module->get_functions()) {
        if (func->get_name() == node->funcName) { f = func; break; }
    }
    if (!f) {
        std::cerr << "Error: Function not found: " << node->funcName << std::endl;
        return ConstantInt::get(0, module); 
    }
    
    std::vector<Value*> args;
    auto funcType = f->get_function_type();
    // 【修改】idx 类型改为 unsigned 匹配 get_num_of_args 返回类型
    unsigned idx = 0;

    for (auto argExp : node->args) {
        Value* val = argExp->accept(*this);
        if (idx < funcType->get_num_of_args()) {
            val = typeCast(val, funcType->get_param_type(idx));
        }
        args.push_back(val);
        idx++;
    }
    return builder->create_call(f, args);
}

Value* IRGenerator::visit(IdExp* node) {
    Value* ptr = scopeMgr.lookup(node->name);
    if (ptr) {
        return builder->create_load(ptr);
    }
    std::cerr << "Error: Unknown variable: " << node->name << std::endl;
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(NumberExp* node) {
    if (node->isFloat) {
        return ConstantFloat::get(node->floatVal, module);
    }
    return ConstantInt::get(node->intVal, module);
}

Value* IRGenerator::visit(FuncFParam* node) { return nullptr; }