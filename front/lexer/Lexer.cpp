#include "Lexer.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>

using namespace std;

Lexer::Lexer(const std::string& f, SymbolTable* st) 
    : filename(f), symTable(st), _hasCached(false), lineNo(1), col(0) {
    file.open(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
    }
    initializeDFA();
    _cachedToken.type = END_OFF;
}

Lexer::~Lexer() {
    if (file.is_open()) file.close();
}

void Lexer::initializeDFA() {
    // 初始化所有状态默认为 Error 或 Start
    // 这里简化处理，未定义的字符如果不处理会走默认逻辑
    
    // 0: Start
    // 1: ID/KW
    // 3: Int
    // 5: Operator (Generic)
    // 6: Separator
    
    // 状态 0 转移
    for (int i = 0; i < 128; i++) {
        char c = (char)i;
        if (isspace(c)) continue; 
        if (isalpha(c) || c == '_') DFA[0].transitions[getInputType(c)] = 1;
        else if (isdigit(c)) DFA[0].transitions[getInputType(c)] = 3;
    }
    
    // 简单符号直接映射
    DFA[1].type = ID;
    DFA[3].type = INT_CONST;
    
    // 为了更准确处理双字符运算符（如 >=, ==），我们在 nextInternal 中特判，
    // DFA 主要用于识别标识符和数字串。
}

int Lexer::getInputType(char ch) {
    if (isalpha(ch)) return 1;
    if (isdigit(ch)) return 2;
    if (ch == '_') return 3;
    return 4; // 其他
}

Token Lexer::nextInternal() {
    while (true) {
        if (col >= currentLine.length()) {
            if (!getline(file, currentLine)) return {END_OFF, "", lineNo};
            currentLine += '\n';
            lineNo++;
            col = 0;
        }

        while (col < currentLine.length() && isspace(currentLine[col])) {
            col++;
        }
        if (col >= currentLine.length()) continue;

        char ch = currentLine[col];
        string buffer;
        buffer += ch;

        // 1. 处理标识符和关键字
        if (isalpha(ch) || ch == '_') {
            col++;
            while (col < currentLine.length()) {
                char c = currentLine[col];
                if (isalnum(c) || c == '_') {
                    buffer += c;
                    col++;
                } else break;
            }
            // 查表
             static std::unordered_map<string, TokenType> kwMap = {
                {"int", KW_INT}, {"void", KW_VOID}, {"return", KW_RETURN},
                {"if", KW_IF}, {"else", KW_ELSE}, {"float", KW_FLOAT},
                {"const", KW_CONST} // {"main", KW_MAIN} 通常 main 也是 ID 或特殊处理
            };
            if (kwMap.count(buffer)) return {kwMap[buffer], buffer, lineNo};
            return {ID, buffer, lineNo};
        }

        // 2. 处理数字
        if (isdigit(ch)) {
            col++;
            while (col < currentLine.length() && isdigit(currentLine[col])) {
                buffer += currentLine[col++];
            }
            return {INT_CONST, buffer, lineNo};
        }

        // 3. 处理运算符和界符 (手动 peek)
        col++; // 已经吃掉了第一个字符
        
        if (ch == '+') return {OP_PLUS, "+", lineNo};
        if (ch == '-') return {OP_MINUS, "-", lineNo};
        if (ch == '*') return {OP_MUL, "*", lineNo};
        if (ch == '/') return {OP_DIV, "/", lineNo};
        if (ch == '%') return {OP_MOD, "%", lineNo};
        
        if (ch == '(') return {SE_LPAREN, "(", lineNo};
        if (ch == ')') return {SE_RPAREN, ")", lineNo};
        if (ch == '{') return {SE_LBRACE, "{", lineNo};
        if (ch == '}') return {SE_RBRACE, "}", lineNo};
        if (ch == ';') return {SE_SEMICOLON, ";", lineNo};
        if (ch == ',') return {SE_COMMA, ",", lineNo};

        if (ch == '=') {
            if (col < currentLine.length() && currentLine[col] == '=') {
                col++; return {OP_EQ, "==", lineNo};
            }
            return {OP_ASSIGN, "=", lineNo};
        }
        if (ch == '>') {
            if (col < currentLine.length() && currentLine[col] == '=') {
                col++; return {OP_GE, ">=", lineNo};
            }
            return {OP_GT, ">", lineNo};
        }
        if (ch == '<') {
            if (col < currentLine.length() && currentLine[col] == '=') {
                col++; return {OP_LE, "<=", lineNo};
            }
            return {OP_LT, "<", lineNo};
        }
        if (ch == '!') {
            if (col < currentLine.length() && currentLine[col] == '=') {
                col++; return {OP_NEQ, "!=", lineNo};
            }
            return {OP_NOT, "!", lineNo};
        }
        if (ch == '&' && col < currentLine.length() && currentLine[col] == '&') {
            col++; return {OP_AND, "&&", lineNo};
        }
        if (ch == '|' && col < currentLine.length() && currentLine[col] == '|') {
            col++; return {OP_OR, "||", lineNo};
        }

        // 未知字符
        return {END_OFF, buffer, lineNo};
    }
}

Token Lexer::next() {
    if (_hasCached) {
        _hasCached = false;
        return _cachedToken;
    }
    return nextInternal();
}

Token Lexer::peek() {
    if (!_hasCached) {
        _cachedToken = nextInternal();
        _hasCached = true;
    }
    return _cachedToken;
}