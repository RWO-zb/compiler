#pragma once

#include <vector>
#include <string>
#include <iostream>

class IRGenerator;
class Value; 

class ASTNode {
public:
    virtual ~ASTNode() = default;
    // 核心接口：Visitor 模式，用于接受 IRGenerator 访问并生成中间代码
    // 返回值 Value* 对应后端的 IR 值（如指令、常量、变量地址等）
    virtual Value* accept(IRGenerator& gen) = 0;
};

// 表达式基类 (Expression)
class Exp : public ASTNode {};

// 语句基类 (Statement)
class Stmt : public ASTNode {};

// 编译单元 (根节点)
class CompUnit : public ASTNode {
public:
    // 存放 Decl (VarDefStmt) 或 FuncDef
    std::vector<ASTNode*> children; 

    Value* accept(IRGenerator& gen) override;
};

// 函数形式参数定义 (例如: int a)
class FuncFParam : public ASTNode {
public:
    std::string type; // "int" 等
    std::string name; // 参数名

    FuncFParam(const std::string& t, const std::string& n) 
        : type(t), name(n) {}
    
    Value* accept(IRGenerator& gen) override;
};

class BlockStmt : public Stmt {
public:
    // 存放语句列表
    std::vector<ASTNode*> stmts; 

    BlockStmt() = default;
    BlockStmt(const std::vector<ASTNode*>& s) : stmts(s) {}
    
    Value* accept(IRGenerator& gen) override;
};

// 函数定义: int main() { ... }
class FuncDef : public ASTNode {
public:
    std::string type; // 返回类型 "int", "void"
    std::string name; // 函数名
    std::vector<FuncFParam*> params; // 参数列表
    BlockStmt* body;  // 函数体

    FuncDef(const std::string& t, const std::string& n, 
            const std::vector<FuncFParam*>& p, BlockStmt* b) 
        : type(t), name(n), params(p), body(b) {}
        
    Value* accept(IRGenerator& gen) override;
};

// 变量定义: int a = 10;
class VarDefStmt : public Stmt {
public:
    std::string type;
    std::string name;
    Exp* initVal; // 初始值表达式，如果没有初始化则为 nullptr

    VarDefStmt(const std::string& t, const std::string& n, Exp* i) 
        : type(t), name(n), initVal(i) {}
        
    Value* accept(IRGenerator& gen) override;
};

// If 语句: if (cond) thenStmt else elseStmt
class IfStmt : public Stmt {
public:
    Exp* cond;
    Stmt* thenStmt;
    Stmt* elseStmt; // 可为 nullptr

    IfStmt(Exp* c, Stmt* t, Stmt* e) 
        : cond(c), thenStmt(t), elseStmt(e) {}
        
    Value* accept(IRGenerator& gen) override;
};

// Return 语句: return exp;
class ReturnStmt : public Stmt {
public:
    Exp* retValue; // 可为 nullptr (如 return;)

    ReturnStmt(Exp* v) : retValue(v) {}
    
    Value* accept(IRGenerator& gen) override;
};

// 二元表达式: a + b, a > b 等
class BinaryExp : public Exp {
public:
    std::string op; // "+", "-", "*", "/", "%", "<", ">=", "==", "&&", "||" 等
    Exp* lhs;
    Exp* rhs;

    BinaryExp(const std::string& o, Exp* l, Exp* r) 
        : op(o), lhs(l), rhs(r) {}
        
    Value* accept(IRGenerator& gen) override;
};

// 函数调用表达式: add(a, b)
class CallExp : public Exp {
public:
    std::string funcName;
    std::vector<Exp*> args;

    CallExp(const std::string& n, const std::vector<Exp*>& a) 
        : funcName(n), args(a) {}
        
    Value* accept(IRGenerator& gen) override;
};

// 数字常量: 123
class NumberExp : public Exp {
public:
    int val;

    NumberExp(int v) : val(v) {}
    
    Value* accept(IRGenerator& gen) override;
};

// 标识符 (引用变量): a
class IdExp : public Exp {
public:
    std::string name;

    IdExp(const std::string& n) : name(n) {}
    
    Value* accept(IRGenerator& gen) override;
};