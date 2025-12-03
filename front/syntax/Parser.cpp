#include "Parser.h"
#include <iostream> 
#include <string>

// 生成叶子节点
// 修改：关键字也返回 IdExp 以便提取类型信息，界符返回 nullptr
ASTNode* Parser::makeLeaf(const Token& tok) {
    switch (tok.type) {
        case INT_CONST:
            return new NumberExp(std::stoi(tok.content));
        case ID:
        case KW_INT:
        case KW_VOID:
        case KW_FLOAT:
            return new IdExp(tok.content); // 将关键字也作为 ID 节点暂存，以便获取文本
        default:
            return nullptr;
    }
}

// 辅助函数：安全获取子节点
ASTNode* getChild(const std::vector<ASTNode*>& children, int index) {
    if (index >= 0 && index < children.size()) return children[index];
    return nullptr;
}

// 构造 AST
ASTNode* Parser::buildAST(const Production& prod, std::vector<ASTNode*>& children) {
    std::string lhs = prod.lhs;
    int len = children.size();

    // 1. Program
    if (lhs == "Program") return getChild(children, 0);

    // 2. CompUnit
    if (lhs == "compUnit") {
        if (len == 2) { // compUnit -> compUnit decl/func
            auto root = dynamic_cast<CompUnit*>(getChild(children, 0));
            ASTNode* item = getChild(children, 1);
            if (!root) root = new CompUnit();
            if (item) root->children.push_back(item);
            return root;
        }
        // compUnit -> decl/func
        auto root = new CompUnit();
        ASTNode* item = getChild(children, 0);
        if (item) root->children.push_back(item);
        return root;
    }

    // 3. FuncDef: funcType ID ( params ) block
    // children: [0:Type, 1:ID, 2:(, 3:Params?, 4:), 5:Block]
    if (lhs == "funcDef") {
        auto typeNode = dynamic_cast<IdExp*>(getChild(children, 0));
        auto idNode = dynamic_cast<IdExp*>(getChild(children, 1));
        std::string retType = typeNode ? typeNode->name : "void";
        std::string name = idNode ? idNode->name : "";
        
        std::vector<FuncFParam*> params;
        // 解析参数列表（如果在 children[3]）
        // 这里的逻辑简化处理：假设 parser 没有把 params 展平，实际可能需要额外的辅助节点
        // 这里假设 children 中已经包含了 FuncFParam 节点或者 Params 列表节点
        // 如果你的文法产生式是 funcFParams -> ...，那么 children[3] 可能是 params 列表的头
        
        // 简化：目前先不处理复杂参数解析，需结合 funcFParams 产生式
        // 实际上 funcFParams 会返回一个包含 params 的节点，这里需要 cast
        
        BlockStmt* body = dynamic_cast<BlockStmt*>(children.back());
        return new FuncDef(retType, name, params, body);
    }

    // 4. VarDef: bType ID ;  或 bType ID = val ;
    // 注意：根据文法 varDecl -> bType varDefList ;
    // 我们这里简化，直接看 varDef -> ID | ID = init
    if (lhs == "varDef") {
        auto idNode = dynamic_cast<IdExp*>(getChild(children, 0));
        if (idNode) {
            Exp* init = nullptr;
            if (len >= 3) init = dynamic_cast<Exp*>(getChild(children, 2));
            // 变量类型在 varDecl 层级，这里暂时默认为 int，IRGen 可能会根据上下文修正
            return new VarDefStmt("int", idNode->name, init); 
        }
    }

    // 5. Block
    if (lhs == "block") { // { blockItems }
        return getChild(children, 1);
    }
    if (lhs == "blockItems") {
        auto list = dynamic_cast<BlockStmt*>(getChild(children, 0));
        auto item = getChild(children, 1);
        if (!list) list = new BlockStmt();
        if (item) {
            if (auto stmt = dynamic_cast<Stmt*>(item)) list->stmts.push_back(stmt);
            // 如果 decl 也是 Stmt 的子类
        }
        return list;
    }

    // 6. Stmt
    if (lhs == "stmt") {
        if (len == 1) return getChild(children, 0); // block | returnStmt...
    }
    if (lhs == "returnStmt") {
        if (len == 3) return new ReturnStmt(dynamic_cast<Exp*>(getChild(children, 1)));
        return new ReturnStmt(nullptr);
    }
    if (lhs == "ifStmt") {
        // if ( cond ) stmt [else stmt]
        auto cond = dynamic_cast<Exp*>(getChild(children, 2));
        auto thenStmt = dynamic_cast<Stmt*>(getChild(children, 4));
        Stmt* elseStmt = nullptr;
        if (len > 5) elseStmt = dynamic_cast<Stmt*>(getChild(children, 6));
        return new IfStmt(cond, thenStmt, elseStmt);
    }

    // 7. Exp & Binary
    if (lhs == "addExp" || lhs == "mulExp" || lhs == "relExp" || lhs == "eqExp") {
        if (len == 1) return getChild(children, 0);
        if (len == 3) {
            auto l = dynamic_cast<Exp*>(getChild(children, 0));
            auto r = dynamic_cast<Exp*>(getChild(children, 2));
            // 操作符文本需要从 prod 获取，或者 children[1] 如果 makeLeaf 处理了
            // 由于 makeLeaf 对 OP 可能会返回 nullptr，我们直接查产生式
            std::string op = prod.rhs[1]; 
            return new BinaryExp(op, l, r);
        }
    }
    
    // 8. Call
    if (lhs == "funcCall") {
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        std::vector<Exp*> args;
        // 参数解析略，需实现 funcRParams
        return new CallExp(id ? id->name : "", args);
    }

    if (lhs == "primaryExp") {
        if (len == 3) return getChild(children, 1); // ( exp )
        return getChild(children, 0);
    }

    // 默认透传
    if (!children.empty()) return children[0];
    return nullptr;
}

ASTNode* Parser::parse() {
    stateStack.push(0);
    while (true) {
        Token lookahead = lexer.peek();
        Action act = slr.getAction(stateStack.top(), lookahead.type);

        if (act.type == Action::SHIFT) {
            stateStack.push(act.target);
            Token t = lexer.next();
            nodeStack.push(makeLeaf(t));
        }
        else if (act.type == Action::REDUCE) {
            Production prod = slr.getProduction(act.target);
            int k = prod.rhsLen();
            std::vector<ASTNode*> children(k);
            for (int i = k - 1; i >= 0; --i) {
                if (!nodeStack.empty()) {
                    children[i] = nodeStack.top();
                    nodeStack.pop();
                } else children[i] = nullptr;
                if (!stateStack.empty()) stateStack.pop();
            }
            ASTNode* newNode = buildAST(prod, children);
            nodeStack.push(newNode);
            
            if (stateStack.empty()) return nullptr;
            int next = slr.getGoto(stateStack.top(), prod.lhs);
            stateStack.push(next);
        }
        else if (act.type == Action::ACCEPT) return nodeStack.top();
        else return nullptr;
    }
}