#include "IRGenerator.h"
#include "compiler_ir/include/Constant.h" 
#include "compiler_ir/include/BasicBlock.h"
#include "compiler_ir/include/Function.h"
#include "compiler_ir/include/GlobalVariable.h"
#include "compiler_ir/include/Type.h"
#include <iostream>
#include <vector>

// ========== 辅助类：作用域管理 ==========
// (保留之前的 ScopeManager 实现，为了完整性再次列出，如果是单文件可直接合并)
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

// ========== IRGenerator 实现 ==========

IRGenerator::IRGenerator(Module* m, SymbolTable* st) : module(m), currentFunc(nullptr) {
    builder = new IRBuilder(nullptr, module);
    scopeMgr = ScopeManager(); // 重置作用域
    globalValues.clear();      // 清空全局数值表
}

// [新增] 常量折叠：递归计算表达式的整数值
int IRGenerator::evaluateConst(ASTNode* node) {
    if (auto num = dynamic_cast<NumberExp*>(node)) {
        return num->val;
    }
    else if (auto id = dynamic_cast<IdExp*>(node)) {
        // 如果是引用变量，查找其在 globalValues 中的值
        if (globalValues.count(id->name)) {
            return globalValues[id->name];
        }
        // 如果是局部变量或未找到，无法在编译期计算 (对于全局初始化这是错误的)
        std::cerr << "Error: Global initializer refers to unknown or non-const variable: " << id->name << std::endl;
        return 0;
    }
    else if (auto bin = dynamic_cast<BinaryExp*>(node)) {
        int l = evaluateConst(bin->lhs);
        int r = evaluateConst(bin->rhs);
        std::string op = bin->op;
        
        if (op == "+" || op == "OP_PLUS") return l + r;
        if (op == "-" || op == "OP_MINUS") return l - r;
        if (op == "*" || op == "OP_MUL") return l * r;
        if (op == "/" || op == "OP_DIV") return (r != 0) ? (l / r) : 0;
        if (op == "%" || op == "OP_MOD") return (r != 0) ? (l % r) : 0;
        // 逻辑运算
        if (op == ">" || op == "OP_GT") return l > r;
        if (op == "<" || op == "OP_LT") return l < r;
        if (op == ">=" || op == "OP_GE") return l >= r;
        if (op == "<=" || op == "OP_LE") return l <= r;
        if (op == "==" || op == "OP_EQ") return l == r;
        if (op == "!=" || op == "OP_NEQ") return l != r;
        if (op == "&&" || op == "OP_AND") return l && r;
        if (op == "||" || op == "OP_OR") return l || r;
    }
    return 0; // 默认
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
        paramTypes.push_back(Type::get_int32_type(module));
    }
    
    Type* retType = (node->type == "void") ? Type::get_void_type(module) : Type::get_int32_type(module);
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
        Value* alloc = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(arg, alloc);
        scopeMgr.declare(p->name, alloc);
        idx++;
    }

    if (node->body) node->body->accept(*this);

    if (!builder->get_insert_block()->get_terminator()) {
        if (node->type == "void") builder->create_void_ret();
        else builder->create_ret(ConstantInt::get(0, module));
    }
    
    scopeMgr.exit();
    currentFunc = nullptr;
    return nullptr;
}

Value* IRGenerator::visit(BlockStmt* node) {
    scopeMgr.enter();
    for (auto stmt : node->stmts) {
        if (stmt) stmt->accept(*this);
    }
    scopeMgr.exit();
    return nullptr;
}

Value* IRGenerator::visit(VarDefStmt* node) {
    if (scopeMgr.isGlobal()) {
        // === [关键修改] 全局变量初始化 ===
        Constant* initConst = nullptr;
        int initValInt = 0;

        if (node->initVal) {
            // 使用 evaluateConst 进行常量折叠，而不是调用 accept
            initValInt = evaluateConst(node->initVal);
            initConst = ConstantInt::get(initValInt, module);
        } else {
            initConst = ConstantInt::get(0, module);
        }

        GlobalVariable* gVar = GlobalVariable::create(node->name, module, Type::get_int32_type(module), false, initConst);
        scopeMgr.declare(node->name, gVar);
        
        // [新增] 记录全局变量的数值，供后续全局变量初始化引用
        globalValues[node->name] = initValInt;

    } else {
        // === 局部变量 ===
        Value* alloc = builder->create_alloca(Type::get_int32_type(module));
        scopeMgr.declare(node->name, alloc);
        
        if (node->initVal) {
            // 局部变量可以使用 accept 生成指令
            Value* v = node->initVal->accept(*this);
            builder->create_store(v, alloc);
        }
    }
    return nullptr;
}

Value* IRGenerator::visit(IfStmt* node) {
    Value* cond = node->cond->accept(*this);
    if (cond->get_type()->is_int32_type()) 
        cond = builder->create_icmp_ne(cond, ConstantInt::get(0, module));

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
    if (node->retValue) {
        Value* v = node->retValue->accept(*this);
        builder->create_ret(v);
    } else {
        builder->create_void_ret();
    }
    return nullptr;
}

Value* IRGenerator::visit(BinaryExp* node) {
    // 赋值
    if (node->op == "=" || node->op == "OP_ASSIGN") {
        auto id = dynamic_cast<IdExp*>(node->lhs);
        Value* ptr = scopeMgr.lookup(id->name);
        if (ptr) {
            Value* v = node->rhs->accept(*this);
            builder->create_store(v, ptr);
            return v;
        }
        return ConstantInt::get(0, module);
    }
    
    // 逻辑运算短路 (&&, ||) 
    if (node->op == "&&" || node->op == "OP_AND") {
        Value* l = node->lhs->accept(*this);
        if (l->get_type()->is_int32_type()) l = builder->create_icmp_ne(l, ConstantInt::get(0, module));
        BasicBlock* rhsBB = BasicBlock::create(module, "and_rhs", currentFunc);
        BasicBlock* endBB = BasicBlock::create(module, "and_end", currentFunc);
        Value* resVar = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(ConstantInt::get(0, module), resVar);
        builder->create_cond_br(l, rhsBB, endBB);
        builder->set_insert_point(rhsBB);
        Value* r = node->rhs->accept(*this);
        if (r->get_type()->is_int32_type()) r = builder->create_icmp_ne(r, ConstantInt::get(0, module));
        builder->create_store(builder->create_zext(r, Type::get_int32_type(module)), resVar);
        builder->create_br(endBB);
        builder->set_insert_point(endBB);
        return builder->create_load(resVar);
    }
    if (node->op == "||" || node->op == "OP_OR") {
        Value* l = node->lhs->accept(*this);
        if (l->get_type()->is_int32_type()) l = builder->create_icmp_ne(l, ConstantInt::get(0, module));
        BasicBlock* rhsBB = BasicBlock::create(module, "or_rhs", currentFunc);
        BasicBlock* endBB = BasicBlock::create(module, "or_end", currentFunc);
        Value* resVar = builder->create_alloca(Type::get_int32_type(module));
        builder->create_store(ConstantInt::get(1, module), resVar);
        builder->create_cond_br(l, endBB, rhsBB);
        builder->set_insert_point(rhsBB);
        Value* r = node->rhs->accept(*this);
        if (r->get_type()->is_int32_type()) r = builder->create_icmp_ne(r, ConstantInt::get(0, module));
        builder->create_store(builder->create_zext(r, Type::get_int32_type(module)), resVar);
        builder->create_br(endBB);
        builder->set_insert_point(endBB);
        return builder->create_load(resVar);
    }

    // 普通运算
    Value* l = node->lhs->accept(*this);
    Value* r = node->rhs->accept(*this);
    
    std::string op = node->op;
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
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(CallExp* node) {
    Function* f = nullptr;
    for (auto func : module->get_functions()) {
        if (func->get_name() == node->funcName) { f = func; break; }
    }
    if (!f) return ConstantInt::get(0, module); 
    
    std::vector<Value*> args;
    for (auto arg : node->args) args.push_back(arg->accept(*this));
    return builder->create_call(f, args);
}

Value* IRGenerator::visit(IdExp* node) {
    Value* ptr = scopeMgr.lookup(node->name);
    if (ptr) return builder->create_load(ptr);
    return ConstantInt::get(0, module);
}

Value* IRGenerator::visit(NumberExp* node) {
    return ConstantInt::get(node->val, module);
}

Value* IRGenerator::visit(FuncFParam* node) { return nullptr; }