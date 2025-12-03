#include "Parser.h"
#include <iostream> 
#include <string>

// ======================================================
// 1. 辅助函数实现
// ======================================================

// 生成叶子节点
ASTNode* Parser::makeLeaf(const Token& tok) {
    switch (tok.type) {
        case INT_CONST:
            return new NumberExp(std::stoi(tok.content));
        case ID:
        case KW_INT:
        case KW_VOID:
        case KW_FLOAT:
            // 将关键字也视为 IdExp，以便在 buildAST 中提取类型名称
            return new IdExp(tok.content); 
        default:
            // 对于运算符等其他 Token，暂时返回 nullptr，
            // 它们会在 buildAST 中通过产生式信息恢复，或者在这里也可以扩展处理
            return nullptr;
    }
}

// 辅助函数：安全获取子节点，防止越界
ASTNode* getChild(const std::vector<ASTNode*>& children, int index) {
    if (index >= 0 && index < children.size()) return children[index];
    return nullptr;
}

// ======================================================
// 2. 核心 AST 构建逻辑 (已修复变量定义和语句处理)
// ======================================================
ASTNode* Parser::buildAST(const Production& prod, std::vector<ASTNode*>& children) {
    std::string lhs = prod.lhs;
    int len = children.size();

    // 1. Program -> compUnit
    if (lhs == "Program") return getChild(children, 0);

    // 2. CompUnit -> compUnit decl | compUnit funcDef ...
    if (lhs == "compUnit") {
        if (len == 2) { 
            auto root = dynamic_cast<CompUnit*>(getChild(children, 0));
            ASTNode* item = getChild(children, 1);
            if (!root) root = new CompUnit();
            if (item) root->children.push_back(item);
            return root;
        }
        // compUnit -> decl | funcDef
        auto root = new CompUnit();
        ASTNode* item = getChild(children, 0);
        if (item) root->children.push_back(item);
        return root;
    }

    // 3. FuncDef -> bType ID ( params ) block
    if (lhs == "funcDef") {
        // Index通常为: 0:Type, 1:ID, 2:(, 3:Params?, 4:), 5:Block
        std::string retType = "void";
        if (auto t = dynamic_cast<IdExp*>(getChild(children, 0))) retType = t->name;
        
        std::string name = "unknown";
        if (auto id = dynamic_cast<IdExp*>(getChild(children, 1))) name = id->name;
        
        std::vector<FuncFParam*> params;
        // 参数处理暂略，需实现 funcFParams 逻辑
        
        BlockStmt* body = dynamic_cast<BlockStmt*>(children.back());
        return new FuncDef(retType, name, params, body);
    }

    // 4. Decl & VarDecl (修复了索引偏移)
    if (lhs == "decl") return getChild(children, 0); // varDecl or constDecl

    if (lhs == "varDecl") {
        // varDecl -> bType varDefList ;
        // 返回 varDefList (index 1) 而不是 bType (index 0)
        return getChild(children, 1);
    }
    
    if (lhs == "constDecl") return getChild(children, 2); 

    if (lhs == "varDefList" || lhs == "constDefList") {
        // 简化：varDefList -> varDef
        // 或者是列表递归，这里简单返回最后一个有效节点
        if (len == 1) return getChild(children, 0);
        return getChild(children, 2); 
    }

    if (lhs == "varDef" || lhs == "constDef") {
        // varDef -> ID | ID = init
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        if (id) {
            Exp* init = nullptr;
            // 如果有初始化: ID = val (len >= 3)
            if (len >= 3) init = dynamic_cast<Exp*>(getChild(children, 2));
            return new VarDefStmt("int", id->name, init); 
        }
    }

    // 5. Block
    if (lhs == "block") return getChild(children, 1); // { blockItems }
    
    if (lhs == "blockItems") {
        auto list = dynamic_cast<BlockStmt*>(getChild(children, 0));
        auto item = getChild(children, 1);
        
        if (!list) list = new BlockStmt();
        if (item) list->stmts.push_back(item);
        return list;
    }
    
    if (lhs == "blockItem") return getChild(children, 0);

    // 6. Stmt
    if (lhs == "stmt") {
        // stmt -> lVal = exp ;
        if (prod.rhs.size() > 1 && prod.rhs[1] == "OP_ASSIGN") {
             return new BinaryExp("=", (Exp*)getChild(children, 0), (Exp*)getChild(children, 2));
        }
        // block, if, return, exp;
        if (len == 1) return getChild(children, 0); 
        if (len == 2) return getChild(children, 0); 
    }
    
    if (lhs == "returnStmt") {
        if (len == 3) return new ReturnStmt(dynamic_cast<Exp*>(getChild(children, 1)));
        return new ReturnStmt(nullptr);
    }
    
    if (lhs == "ifStmt") {
        auto cond = dynamic_cast<Exp*>(getChild(children, 2));
        auto thenStmt = dynamic_cast<Stmt*>(getChild(children, 4));
        Stmt* elseStmt = nullptr;
        if (len > 5) elseStmt = dynamic_cast<Stmt*>(getChild(children, 6));
        return new IfStmt(cond, thenStmt, elseStmt);
    }

    // 7. Expressions
    if (lhs == "addExp" || lhs == "mulExp" || lhs == "relExp" || lhs == "eqExp") {
        if (len == 1) return getChild(children, 0);
        if (len == 3) {
            std::string op = prod.rhs[1]; // 从产生式获取操作符
            
            // 如果 makeLeaf 返回了操作符的 Leaf 节点，尝试从节点获取
            if (auto opNode = dynamic_cast<IdExp*>(getChild(children, 1))) {
                op = opNode->name;
            } else {
                // 如果 Token 被丢弃了(nullptr)，手动映射常见操作符
                if (op == "addOp") op = "+"; // 简化处理
                // 实际上应该通过产生式具体的 rhs 符号来判断
                // 例如: addExp -> mulExp OP_PLUS mulExp
            }
            return new BinaryExp(op, (Exp*)getChild(children, 0), (Exp*)getChild(children, 2));
        }
    }
    
    // 传递操作符
    if (lhs == "addOp" || lhs == "mulOp" || lhs == "relOp" || lhs == "eqOp") {
        // 假设 children[0] 是 OP Token 转换的 IdExp
        // 如果 makeLeaf 返回 nullptr，这里需要手动根据 prod.rhs[0] 创建 IdExp
        if (getChild(children, 0)) return getChild(children, 0);
        return new IdExp(prod.rhs[0]); // 使用产生式右部符号作为名称
    }

    if (lhs == "funcCall") {
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        std::vector<Exp*> args;
        return new CallExp(id ? id->name : "", args);
    }

    if (lhs == "primaryExp") {
        if (len == 3) return getChild(children, 1); // ( exp )
        return getChild(children, 0);
    }
    
    if (lhs == "initVal") return getChild(children, 0);

    // 默认透传
    if (!children.empty()) return children[0];
    return nullptr;
}

// ======================================================
// 3. 解析主循环 (之前丢失的部分！)
// ======================================================
ASTNode* Parser::parse() {
    stateStack.push(0); // 初始状态
    while (true) {
        Token lookahead = lexer.peek();
        Action act = slr.getAction(stateStack.top(), lookahead.type);

        if (act.type == Action::SHIFT) {
            // 移进：压入状态，消耗 Token
            stateStack.push(act.target);
            Token t = lexer.next();
            // 将 Token 转换为 AST 叶子节点压栈
            nodeStack.push(makeLeaf(t));
        }
        else if (act.type == Action::REDUCE) {
            // 规约：弹出栈元素，构建新 AST 节点
            Production prod = slr.getProduction(act.target);
            int k = prod.rhsLen();
            
            std::vector<ASTNode*> children(k);
            for (int i = k - 1; i >= 0; --i) {
                if (!nodeStack.empty()) {
                    children[i] = nodeStack.top();
                    nodeStack.pop();
                } else {
                    children[i] = nullptr;
                }
                
                if (!stateStack.empty()) stateStack.pop();
            }
            
            // 调用构建逻辑
            ASTNode* newNode = buildAST(prod, children);
            nodeStack.push(newNode);
            
            // GOTO 跳转
            if (stateStack.empty()) return nullptr; // Error
            int next = slr.getGoto(stateStack.top(), prod.lhs);
            stateStack.push(next);
        }
        else if (act.type == Action::ACCEPT) {
            // 接受
            return nodeStack.top();
        }
        else {
            // 错误
            // std::cerr << "Syntax error at line " << lookahead.line << std::endl;
            return nullptr;
        }
    }
}