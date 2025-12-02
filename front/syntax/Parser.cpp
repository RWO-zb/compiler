#include "Parser.h"

// 生成一个终结符 AST 节点（很多不会生成真正节点，而是返回 nullptr）
ASTNode* Parser::makeLeaf(const Token& tok) {
    switch (tok.type) {
        case INT_CONST:
            return new NumberExp(std::stoi(tok.content));
        case ID:
            return new IdExp(tok.content);
        default:
            // 其他终结符在语义上无意义
            return nullptr;
    }
}


// 根据产生式构造 AST
ASTNode* Parser::buildAST(const Production& prod,
                          std::vector<ASTNode*>& children)
{
    int id = prod.id;

    switch (id) {

    // ---------------------- Program 系 ----------------------
    case 1: // Program -> compUnit
        return children[0];

    // ---------------------- compUnit 系 --------------------
    case 2: // compUnit -> compUnit decl
    case 2*1000+1: // 手动定义（重复规则）
        {
            auto root = dynamic_cast<CompUnit*>(children[0]);
            root->children.push_back(children[1]);
            return root;
        }
    case 2001: // compUnit -> funcDef
        {
            auto cu = new CompUnit();
            cu->children.push_back(children[0]);
            return cu;
        }

    // ---------------------- decl / varDecl -------------------
    case 8: // varDecl -> bType varDef
        return children[1];

    case 9: // varDef -> Ident
        return new VarDefStmt("int", 
                              dynamic_cast<IdExp*>(children[0])->name,
                              nullptr);
    case 901: // varDef -> Ident '=' initVal
        return new VarDefStmt("int",
                              dynamic_cast<IdExp*>(children[0])->name,
                              children[2]);

    // ---------------------- initVal -> exp -------------------
    case 10:
        return children[0];

    // ---------------------- exp 系 ---------------------------
    case 18: // exp -> addExp
        return children[0];

    // addExp 递归右结合
    case 28: // addExp -> addExp + mulExp
        return new BinaryExp("+", children[0], children[2]);
    case 281: // addExp -> addExp - mulExp
        return new BinaryExp("-", children[0], children[2]);
    case 280: // addExp -> mulExp
        return children[0];

    // mulExp
    case 27: // mulExp -> mulExp * unaryExp
        return new BinaryExp("*", children[0], children[2]);
    case 271:  // mulExp -> mulExp / unaryExp
        return new BinaryExp("/", children[0], children[2]);
    case 272:  // mulExp -> mulExp % unaryExp
        return new BinaryExp("%", children[0], children[2]);
    case 270: // mulExp -> unaryExp
        return children[0];

    // primaryExp
    case 21: // number | Ident | '(' exp ')'
        return children[0];

    // stmt = lVal '=' exp ';'
    case 17:
        return new BinaryExp("=",
                             children[0],  // lVal
                             children[2]); // exp

    // if / return / block
    case 171: // return exp ;
        return new ReturnStmt(children[1]);
    case 172: // return ;
        return new ReturnStmt(nullptr);

    case 15: // block -> { stmts }
        return children[1];

    case 20: // lVal -> Ident
        return children[0];

    }

    return nullptr;
}



// --------------------- Parser 主流程 ---------------------

ASTNode* Parser::parse() {
    stateStack.push(0);

    while (true) {
        Token lookahead = lexer.peek();
        Action act = slr.getAction(stateStack.top(), lookahead.type);

        // ---------------- SHIFT -----------------
        if (act.type == Action::SHIFT) {
            std::cout << stateStack.top()
                      << "#" << lookahead.content
                      << "\tmove\n";

            stateStack.push(act.target);

            Token t = lexer.next();
            nodeStack.push(makeLeaf(t));

            continue;
        }

        // ---------------- REDUCE -----------------
        else if (act.type == Action::REDUCE) {
            Production prod = slr.getProduction(act.target);
            int k = prod.rhsLen;

            std::cout << stateStack.top()
                      << "#" << lookahead.content
                      << "\treduction\n";

            // 取出 RHS
            std::vector<ASTNode*> children(k);
            for (int i=k-1;i>=0;--i) {
                children[i] = nodeStack.top();
                nodeStack.pop();
                stateStack.pop();
            }

            // 构造父节点
            ASTNode* newNode = buildAST(prod, children);

            nodeStack.push(newNode);

            // goto
            int st = stateStack.top();
            int next = slr.getGoto(st, prod.lhs);
            if (next < 0) {
                std::cerr << "Goto Error\n";
                return nullptr;
            }
            stateStack.push(next);

            continue;
        }

        // ---------------- ACCEPT -----------------
        else if (act.type == Action::ACCEPT) {
            std::cout << "accept\n";
            return nodeStack.top();
        }

        // ---------------- ERROR -------------------
        else {
            std::cerr << "Syntax Error at line " << lookahead.line << "\n";
            return nullptr;
        }
    }
}
