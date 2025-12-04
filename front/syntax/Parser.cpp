#include "Parser.h"
#include <iostream> 
#include <string>
#include <vector>

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
            return nullptr;
    }
}

// 辅助函数：安全获取子节点
ASTNode* getChild(const std::vector<ASTNode*>& children, int index) {
    if (index >= 0 && index < children.size()) return children[index];
    return nullptr;
}

// ======================================================
// 2. 核心 AST 构建逻辑
// ======================================================
ASTNode* Parser::buildAST(const Production& prod, std::vector<ASTNode*>& children) {
    std::string lhs = prod.lhs;
    int len = children.size();

    // 1. Program -> compUnit
    if (lhs == "Program") return getChild(children, 0);

    // 2. CompUnit
    if (lhs == "compUnit") {
        if (len == 2) { 
            auto root = dynamic_cast<CompUnit*>(getChild(children, 0));
            ASTNode* item = getChild(children, 1);
            if (!root) root = new CompUnit();
            if (item) root->children.push_back(item);
            return root;
        }
        auto root = new CompUnit();
        ASTNode* item = getChild(children, 0);
        if (item) root->children.push_back(item);
        return root;
    }

    // 3. FuncDef
    if (lhs == "funcDef") {
        std::string retType = "void";
        if (auto t = dynamic_cast<IdExp*>(getChild(children, 0))) retType = t->name;
        
        std::string name = "unknown";
        if (auto id = dynamic_cast<IdExp*>(getChild(children, 1))) name = id->name;
        
        std::vector<FuncFParam*> params;
        BlockStmt* body = dynamic_cast<BlockStmt*>(children.back());
        return new FuncDef(retType, name, params, body);
    }

    // 4. Decl & VarDecl
    if (lhs == "decl") return getChild(children, 0); 

    if (lhs == "varDecl") return getChild(children, 1);
    
    if (lhs == "constDecl") return getChild(children, 2); 

    if (lhs == "varDefList" || lhs == "constDefList") {
        if (len == 1) return getChild(children, 0);
        return getChild(children, 2); 
    }

    if (lhs == "varDef" || lhs == "constDef") {
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        if (id) {
            Exp* init = nullptr;
            if (len >= 3) init = dynamic_cast<Exp*>(getChild(children, 2));
            return new VarDefStmt("int", id->name, init); 
        }
    }

    // 5. Block
    if (lhs == "block") return getChild(children, 1); 
    
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
        if (prod.rhs.size() > 1 && prod.rhs[1] == "OP_ASSIGN") {
             return new BinaryExp("=", (Exp*)getChild(children, 0), (Exp*)getChild(children, 2));
        }
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
            std::string op = prod.rhs[1];
            if (auto opNode = dynamic_cast<IdExp*>(getChild(children, 1))) {
                op = opNode->name;
            } else {
                if (op == "addOp") op = "+"; 
            }
            return new BinaryExp(op, (Exp*)getChild(children, 0), (Exp*)getChild(children, 2));
        }
    }
    
    if (lhs == "addOp" || lhs == "mulOp" || lhs == "relOp" || lhs == "eqOp") {
        if (getChild(children, 0)) return getChild(children, 0);
        return new IdExp(prod.rhs[0]); 
    }

    if (lhs == "funcCall") {
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        std::vector<Exp*> args;
        return new CallExp(id ? id->name : "", args);
    }

    if (lhs == "primaryExp") {
        if (len == 3) return getChild(children, 1); 
        return getChild(children, 0);
    }
    
    if (lhs == "initVal") return getChild(children, 0);

    if (!children.empty()) return children[0];
    return nullptr;
}

// ======================================================
// 3. 解析主循环 (包含符号映射修改)
// ======================================================
ASTNode* Parser::parse() {
    stateStack.push(0); // 初始状态
    
    // 符号栈：用于输出展示，初始为 #
    std::vector<std::string> symbolStack;
    symbolStack.push_back("#"); 

    int stepCount = 1; // 步骤序号

    while (true) {
        Token lookahead = lexer.peek();
        Action act = slr.getAction(stateStack.top(), lookahead.type);
        
        // --- 准备输出信息 ---
        std::string stackTopSym = symbolStack.back();
        
        std::string inputSym = lookahead.content;
        if (lookahead.type == END_OFF) inputSym = ""; 
        
        std::string actionStr;
        if (act.type == Action::SHIFT) actionStr = "move"; 
        else if (act.type == Action::REDUCE) actionStr = "reduction";
        else if (act.type == Action::ACCEPT) actionStr = "accept";
        else actionStr = "error";

        // 输出格式: [序号] [TAB] [栈顶符号]#[面临输入符号] [TAB] [动作]
        std::cout << stepCount++ << "\t" << stackTopSym << "#" << inputSym << "\t" << actionStr << std::endl;
        // ------------------

        if (act.type == Action::SHIFT) {
            // 移进
            stateStack.push(act.target);
            Token t = lexer.next();
            nodeStack.push(makeLeaf(t));
            
            // [关键修改]：根据 Token 类型入栈标准符号名，而非原始字符串
            // 这样栈顶就会显示 Ident#... 而不是 a#...
            if (t.type == ID) {
                symbolStack.push_back("Ident");
            } else if (t.type == INT_CONST) {
                symbolStack.push_back("IntConst"); 
            } else if (t.type == FLOAT_CONST) {
                symbolStack.push_back("FloatConst");
            } else {
                symbolStack.push_back(t.content); // 关键字和符号保持原样
            }
        }
        else if (act.type == Action::REDUCE) {
            // 规约
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
                
                // 符号出栈
                if (symbolStack.size() > 1) symbolStack.pop_back(); 
            }
            
            // 产生式左部符号入栈
            symbolStack.push_back(prod.lhs);

            ASTNode* newNode = buildAST(prod, children);
            nodeStack.push(newNode);
            
            if (stateStack.empty()) return nullptr;
            int next = slr.getGoto(stateStack.top(), prod.lhs);
            stateStack.push(next);
        }
        else if (act.type == Action::ACCEPT) {
            return nodeStack.top();
        }
        else {
            return nullptr;
        }
    }
}