#include "Parser.h"
#include <iostream> 
#include <string>
#include <vector>

// 生成叶子节点
ASTNode* Parser::makeLeaf(const Token& tok) {
    switch (tok.type) {
        case INT_CONST:
            return new NumberExp(std::stoi(tok.content));
        case FLOAT_CONST:
             // 暂时用整数存储，实际项目建议扩展 AST 支持浮点
            return new NumberExp(std::stoi(tok.content)); 
        case ID:
        case KW_INT:
        case KW_VOID:
        case KW_FLOAT:
        case KW_MAIN: // [关键] main 作为关键字，但在 AST 中被视为 IdExp (标识符表达式)
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

ASTNode* Parser::buildAST(const Production& prod, std::vector<ASTNode*>& children) {
    std::string lhs = prod.lhs;
    int len = children.size();

    // 1. Program -> compUnit
    if (lhs == "Program") return getChild(children, 0);

    // 2. CompUnit -> compUnit (decl|funcDef) | decl | funcDef
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

    // 3. FuncDef -> bType Ident '(' funcFParamsOpt ')' block
    if (lhs == "funcDef") {
        std::string retType = "void";
        if (auto t = dynamic_cast<IdExp*>(getChild(children, 0))) retType = t->name;
        
        std::string name = "unknown";
        if (auto id = dynamic_cast<IdExp*>(getChild(children, 1))) name = id->name;
        
        std::vector<FuncFParam*> params;
        // 提取参数列表
        if (auto listNode = dynamic_cast<CompUnit*>(getChild(children, 3))) {
            for (auto child : listNode->children) {
                if (auto p = dynamic_cast<FuncFParam*>(child)) {
                    params.push_back(p);
                }
            }
        }

        BlockStmt* body = dynamic_cast<BlockStmt*>(children.back());
        return new FuncDef(retType, name, params, body);
    }

    // 处理形参列表
    if (lhs == "funcFParams") {
        if (len == 1) { // funcFParam
            auto list = new CompUnit();
            if (auto p = getChild(children, 0)) list->children.push_back(p);
            return list;
        } else if (len == 3) { // funcFParams , funcFParam
            auto list = dynamic_cast<CompUnit*>(getChild(children, 0));
            if (!list) list = new CompUnit();
            if (auto p = getChild(children, 2)) list->children.push_back(p);
            return list;
        }
    }
    if (lhs == "funcFParam") {
        std::string type = "int";
        if (auto t = dynamic_cast<IdExp*>(getChild(children, 0))) type = t->name;
        std::string name = "";
        if (auto id = dynamic_cast<IdExp*>(getChild(children, 1))) name = id->name;
        return new FuncFParam(type, name);
    }
    if (lhs == "funcFParamsOpt") {
        if (children.empty()) return new CompUnit();
        return getChild(children, 0);
    }

    // 4. Decl & VarDecl
    if (lhs == "decl") return getChild(children, 0); 
    if (lhs == "varDecl") return getChild(children, 1); 
    if (lhs == "constDecl") return getChild(children, 2); 

    // 变量定义列表
    if (lhs == "varDefList" || lhs == "constDefList") {
        if (len == 1) {
            auto list = new CompUnit();
            list->children.push_back(getChild(children, 0));
            return list;
        } else if (len == 3) {
            auto list = dynamic_cast<CompUnit*>(getChild(children, 0));
            if (!list) list = new CompUnit();
            list->children.push_back(getChild(children, 2));
            return list;
        }
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
        if (len == 0) return new BlockStmt();
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
    if (lhs == "addExp" || lhs == "mulExp" || lhs == "relExp" || lhs == "eqExp" || lhs == "lAndExp" || lhs == "lOrExp") {
        if (len == 1) return getChild(children, 0);
        if (len == 3) {
            std::string op = "unknown";
            if (auto opNode = dynamic_cast<IdExp*>(getChild(children, 1))) {
                op = opNode->name;
            } 
            return new BinaryExp(op, (Exp*)getChild(children, 0), (Exp*)getChild(children, 2));
        }
    }
    
    if (lhs == "addOp" || lhs == "mulOp" || lhs == "relOp" || lhs == "eqOp") {
        if (getChild(children, 0)) return getChild(children, 0);
        if (!prod.rhs.empty()) return new IdExp(prod.rhs[0]);
    }

    if (lhs == "funcCall") {
        auto id = dynamic_cast<IdExp*>(getChild(children, 0));
        std::vector<Exp*> args;
        if (auto listNode = dynamic_cast<CompUnit*>(getChild(children, 2))) {
            for (auto child : listNode->children) {
                if (auto e = dynamic_cast<Exp*>(child)) args.push_back(e);
            }
        }
        return new CallExp(id ? id->name : "", args);
    }

    if (lhs == "funcRParams") {
        if (len == 1) { 
            auto list = new CompUnit();
            if (auto e = getChild(children, 0)) list->children.push_back(e);
            return list;
        } else if (len == 3) { 
            auto list = dynamic_cast<CompUnit*>(getChild(children, 0));
            if (!list) list = new CompUnit();
            if (auto e = getChild(children, 2)) list->children.push_back(e);
            return list;
        }
    }
    if (lhs == "funcRParamsOpt") {
        if (children.empty()) return new CompUnit();
        return getChild(children, 0);
    }

    if (lhs == "primaryExp") {
        if (len == 3) return getChild(children, 1); 
        return getChild(children, 0); 
    }
    
    if (lhs == "initVal") return getChild(children, 0);
    if (lhs == "lVal") return getChild(children, 0);
    if (lhs == "unaryExp") {
         if(len==1) return getChild(children, 0);
         // 处理 funcCall 或 unaryOp
         if(prod.rhs[0] == "funcCall") return getChild(children, 0);
    }
    
    if (!children.empty()) return children[0];
    return nullptr;
}

ASTNode* Parser::parse() {
    stateStack.push(0); 
    
    std::vector<std::string> symbolStack;
    symbolStack.push_back("#"); 

    int stepCount = 1; 

    while (true) {
        Token lookahead = lexer.peek();
        TokenType type = lookahead.type;
        
        // =========================================================
        // [核心修复] 处理 main 关键字问题
        // 文法期望 ID (标识符)，但 Lexer 返回 KW_MAIN (关键字)。
        // 如果当前状态下 KW_MAIN 报错，但 ID 是合法的，则强制将其视为 ID 处理。
        // =========================================================
        Action act = slr.getAction(stateStack.top(), type);
        
        if (act.type == Action::ERROR && type == KW_MAIN) {
            Action idAct = slr.getAction(stateStack.top(), ID);
            if (idAct.type != Action::ERROR) {
                type = ID; // 欺骗 Parser，将其视为 ID
                act = idAct; // 使用 ID 的动作
            }
        }
        // =========================================================

        std::string stackTopSym = symbolStack.back();
        std::string inputSym = (lookahead.type == END_OFF) ? "$" : lookahead.content;
        
        std::string actionStr;
        if (act.type == Action::SHIFT) actionStr = "move"; 
        else if (act.type == Action::REDUCE) actionStr = "reduction";
        else if (act.type == Action::ACCEPT) actionStr = "accept";
        else actionStr = "error";

        std::cout << stepCount++ << "\t" << stackTopSym << "#" << inputSym << "\t" << actionStr << std::endl;

        if (act.type == Action::SHIFT) {
            stateStack.push(act.target);
            Token t = lexer.next(); // 消耗 Token
            nodeStack.push(makeLeaf(t)); 
            
            if (t.type == ID || type == ID) symbolStack.push_back("Ident"); // 注意这里用 type 判断
            else if (t.type == INT_CONST) symbolStack.push_back("IntConst"); 
            else if (t.type == FLOAT_CONST) symbolStack.push_back("FloatConst");
            else symbolStack.push_back(t.content); 
        }
        else if (act.type == Action::REDUCE) {
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
                if (symbolStack.size() > 1) symbolStack.pop_back(); 
            }
            
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
            std::cerr << "Syntax error at line " << lookahead.line << ": unexpected token " << lookahead.content << std::endl;
            return nullptr;
        }
    }
}