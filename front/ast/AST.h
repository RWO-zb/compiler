#pragma once
#include <vector>
#include <string>
#include "../../compiler_ir/include/Value.h" 

class IRGenerator; // 前置声明
class Value;

// AST 基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    // [Role C] 核心：接受 Visitor
    virtual Value* accept(IRGenerator& visitor) = 0;
};

// --- 表达式节点 ---
class ExprNode : public ASTNode {};

class BinaryExp : public ExprNode {
public:
    std::string op; // "+", "-", "==" 等
    ASTNode *lhs, *rhs;
    BinaryExp(std::string o, ASTNode* l, ASTNode* r) : op(o), lhs(l), rhs(r) {}
    Value* accept(IRGenerator& visitor) override;
};

class NumberExp : public ExprNode {
public:
    int val;
    NumberExp(int v) : val(v) {}
    Value* accept(IRGenerator& visitor) override;
};

class IdExp : public ExprNode {
public:
    std::string name;
    IdExp(std::string n) : name(n) {}
    Value* accept(IRGenerator& visitor) override;
};

// --- 语句节点 ---
class StmtNode : public ASTNode {};

class BlockStmt : public StmtNode {
public:
    std::vector<ASTNode*> stmts;
    Value* accept(IRGenerator& visitor) override;
};

class IfStmt : public StmtNode {
public:
    ASTNode *cond, *thenBlock, *elseBlock;
    IfStmt(ASTNode* c, ASTNode* t, ASTNode* e) : cond(c), thenBlock(t), elseBlock(e) {}
    Value* accept(IRGenerator& visitor) override;
};

class ReturnStmt : public StmtNode {
public:
    ASTNode *retVal;
    ReturnStmt(ASTNode* v) : retVal(v) {}
    Value* accept(IRGenerator& visitor) override;
};

class VarDefStmt : public StmtNode {
public:
    std::string type;
    std::string name;
    ASTNode* initVal; // 可为 nullptr
    VarDefStmt(std::string t, std::string n, ASTNode* i) : type(t), name(n), initVal(i) {}
    Value* accept(IRGenerator& visitor) override;
};

class FuncDef : public ASTNode {
public:
    std::string retType;
    std::string name;
    // vector<string> params; // 参数表
    BlockStmt* body;
    Value* accept(IRGenerator& visitor) override;
};

// 根节点
class CompUnit : public ASTNode {
public:
    std::vector<ASTNode*> children; // 全局变量或函数
    Value* accept(IRGenerator& visitor) override;
};