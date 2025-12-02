#include <iostream>
#include <iomanip>
#include "SLRGenerator.h"
#include "SLRGenerator.cpp"

// 辅助函数：将TokenType转换为字符串
std::string tokenTypeToString(TokenType type) {
    static std::map<TokenType, std::string> tokenNames = {
        {ID, "ID"},
        {INT_CONST, "INT_CONST"},
        {FLOAT_CONST, "FLOAT_CONST"},
        {KW_INT, "KW_INT"},
        {KW_VOID, "KW_VOID"},
        {KW_RETURN, "KW_RETURN"},
        {KW_CONST, "KW_CONST"},
        {KW_MAIN, "KW_MAIN"},
        {KW_FLOAT, "KW_FLOAT"},
        {KW_IF, "KW_IF"},
        {KW_ELSE, "KW_ELSE"},
        {OP_PLUS, "OP_PLUS"},
        {OP_MINUS, "OP_MINUS"},
        {OP_MUL, "OP_MUL"},
        {OP_DIV, "OP_DIV"},
        {OP_MOD, "OP_MOD"},
        {OP_ASSIGN, "OP_ASSIGN"},
        {OP_NOT, "OP_NOT"},
        {OP_EQ, "OP_EQ"},
        {OP_NEQ, "OP_NEQ"},
        {OP_LT, "OP_LT"},
        {OP_GT, "OP_GT"},
        {OP_LE, "OP_LE"},
        {OP_GE, "OP_GE"},
        {OP_AND, "OP_AND"},
        {OP_OR, "OP_OR"},
        {SE_LPAREN, "SE_LPAREN"},
        {SE_RPAREN, "SE_RPAREN"},
        {SE_LBRACE, "SE_LBRACE"},
        {SE_RBRACE, "SE_RBRACE"},
        {SE_SEMICOLON, "SE_SEMICOLON"},
        {SE_COMMA, "SE_COMMA"},
        {END_OFF, "END_OFF"}
    };
    
    auto it = tokenNames.find(type);
    if (it != tokenNames.end()) {
        return it->second;
    }
    return "UNKNOWN(" + std::to_string(static_cast<int>(type)) + ")";
}

// 详细测试FIRST和FOLLOW集
void testFirstFollowSets(SLRGenerator& generator) {
    std::cout << "\n=== Detailed FIRST/FOLLOW Test ===\n";
    
    const auto& firstSets = generator.getFirstSets();
    const auto& followSets = generator.getFollowSets();
    
    // 测试一些关键非终结符
    std::vector<std::string> testNonTerms = {
        "Program", "compUnit", "decl", "constDecl", "varDecl",
        "btype", "exp", "stmt", "block", "funcDef"
    };
    
    std::cout << std::left << std::setw(15) << "Non-terminal" 
              << std::setw(30) << "FIRST Set" 
              << "FOLLOW Set\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& nt : testNonTerms) {
        std::cout << std::setw(15) << nt;
        
        // 输出FIRST集
        std::string firstStr = "{ ";
        if (firstSets.find(nt) != firstSets.end()) {
            for (TokenType token : firstSets.at(nt)) {
                firstStr += tokenTypeToString(token) + " ";
            }
        }
        firstStr += "}";
        std::cout << std::setw(30) << firstStr;
        
        // 输出FOLLOW集
        std::string followStr = "{ ";
        if (followSets.find(nt) != followSets.end()) {
            for (TokenType token : followSets.at(nt)) {
                followStr += tokenTypeToString(token) + " ";
            }
        }
        followStr += "}";
        std::cout << followStr << "\n";
    }
    
    // 验证一些重要属性
    std::cout << "\n=== Validation Checks ===\n";
    
    // 检查btype
    if (firstSets.find("btype") != firstSets.end()) {
        const auto& btypeFirst = firstSets.at("btype");
        bool hasInt = btypeFirst.find(KW_INT) != btypeFirst.end();
        bool hasFloat = btypeFirst.find(KW_FLOAT) != btypeFirst.end();
        
        std::cout << "btype FIRST contains KW_INT: " << (hasInt ? "✓" : "✗") << "\n";
        std::cout << "btype FIRST contains KW_FLOAT: " << (hasFloat ? "✓" : "✗") << "\n";
        
        if (!hasInt || !hasFloat) {
            std::cout << "ERROR: btype FIRST set is incorrect!\n";
        }
    }
    
    // 检查exp的FOLLOW应该包含分号等
    if (followSets.find("exp") != followSets.end()) {
        const auto& expFollow = followSets.at("exp");
        bool hasSemicolon = expFollow.find(SE_SEMICOLON) != expFollow.end();
        bool hasRParen = expFollow.find(SE_RPAREN) != expFollow.end();
        bool hasComma = expFollow.find(SE_COMMA) != expFollow.end();
        
        std::cout << "exp FOLLOW contains SE_SEMICOLON: " << (hasSemicolon ? "✓" : "✗") << "\n";
        std::cout << "exp FOLLOW contains SE_RPAREN: " << (hasRParen ? "✓" : "✗") << "\n";
        std::cout << "exp FOLLOW contains SE_COMMA: " << (hasComma ? "✓" : "✗") << "\n";
    }
}

// 测试产生式
void testProductions(SLRGenerator& generator) {
    std::cout << "\n=== Production Test ===\n";
    
    const auto& prods = generator.getProductions();
    std::cout << "Total productions: " << prods.size() << "\n";
    
    // 显示前10个产生式
    std::cout << "\nFirst 10 productions:\n";
    for (int i = 0; i < std::min(10, (int)prods.size()); i++) {
        const auto& prod = prods[i];
        std::cout << i << ". " << prod.lhs << " -> ";
        if (prod.rhs.empty()) {
            std::cout << "ε";
        } else {
            for (const auto& sym : prod.rhs) {
                std::cout << sym << " ";
            }
        }
        std::cout << "\n";
    }
    
    // 检查增广文法
    if (!prods.empty() && prods[0].lhs == "S'" && prods[0].rhs.size() == 1 && prods[0].rhs[0] == "Program") {
        std::cout << "\n✓ Augmented grammar (S' -> Program) is production 0\n";
    } else {
        std::cout << "\n✗ ERROR: Augmented grammar missing or incorrect!\n";
    }
}

// 测试ε推导
void testEpsilonDeriving(SLRGenerator& generator) {
    std::cout << "\n=== ε-Derivation Test ===\n";
    
    const auto& epsilonNonTerms = generator.getEpsilonDerivingNonTerms();
    std::cout << "Non-terminals that can derive ε: ";
    
    if (epsilonNonTerms.empty()) {
        std::cout << "(none)";
    } else {
        for (const auto& nt : epsilonNonTerms) {
            std::cout << nt << " ";
        }
    }
    std::cout << "\n";
    
    // 根据文法，这些应该能推导ε
    std::vector<std::string> shouldDeriveEpsilon = {
        "compUnitItem", "funcFParamsOpt", "blockItems", 
        "expOpt", "elseOpt", "funcRParamsOpt"
    };
    
    std::cout << "\nChecking expected ε-deriving non-terminals:\n";
    for (const auto& nt : shouldDeriveEpsilon) {
        bool canDerive = epsilonNonTerms.find(nt) != epsilonNonTerms.end();
        std::cout << nt << ": " << (canDerive ? "✓" : "✗") << "\n";
    }
}

// 测试SLR表查询
void testSLRTableQueries(SLRGenerator& generator) {
    std::cout << "\n=== SLR Table Query Test ===\n";
    
    // 测试一些基本查询
    std::cout << "Testing basic SLR table queries:\n";
    
    // 测试状态0的移进动作
    Action action = generator.getAction(0, ID);
    std::cout << "State 0, token ID: ";
    if (action.type == Action::SHIFT) {
        std::cout << "shift to state " << action.target << " ✓\n";
    } else if (action.type == Action::ERROR) {
        std::cout << "error (might be normal)\n";
    } else {
        std::cout << "unexpected action type: " << action.type << "\n";
    }
    
    // 测试goto
    int gotoState = generator.getGoto(0, "Program");
    if (gotoState != -1) {
        std::cout << "goto(0, Program) = state " << gotoState << " ✓\n";
    } else {
        std::cout << "goto(0, Program) = -1 (might be normal)\n";
    }
    
    // 测试产生式查询
    try {
        Production prod = generator.getProduction(0);
        std::cout << "Production 0: " << prod.lhs << " -> ";
        for (const auto& sym : prod.rhs) std::cout << sym << " ";
        std::cout << "✓\n";
    } catch (const std::exception& e) {
        std::cout << "Error getting production 0: " << e.what() << "\n";
    }
}

int main() {
    try {
        std::cout << "========================================\n";
        std::cout << "     SLR Table Generator - Full Test    \n";
        std::cout << "========================================\n\n";
        
        SLRGenerator generator;
        
        std::cout << "Step 1: Building SLR table...\n";
        std::cout << "----------------------------------------\n";
        
        generator.build();
        
        std::cout << "\n========================================\n";
        std::cout << "         Additional Tests              \n";
        std::cout << "========================================\n";
        
        // 运行额外测试
        testFirstFollowSets(generator);
        testProductions(generator);
        testEpsilonDeriving(generator);
        testSLRTableQueries(generator);
        
        std::cout << "\n========================================\n";
        std::cout << "           All Tests Completed!        \n";
        std::cout << "========================================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n!!! ERROR: " << e.what() << "\n";
        return 1;
    }
}