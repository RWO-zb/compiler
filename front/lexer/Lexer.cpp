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
    // DFA 用于辅助，主要逻辑在 nextInternal
    for (int i = 0; i < 128; i++) {
        char c = (char)i;
        if (isspace(c)) continue; 
        if (isalpha(c) || c == '_') DFA[0].transitions[getInputType(c)] = 1;
        else if (isdigit(c)) DFA[0].transitions[getInputType(c)] = 3;
    }
    DFA[1].type = ID;
    DFA[3].type = INT_CONST;
}

int Lexer::getInputType(char ch) {
    if (isalpha(ch)) return 1;
    if (isdigit(ch)) return 2;
    if (ch == '_') return 3;
    return 4; 
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

        // === 注释处理 ===
        // 1. 单行注释 //
        if (ch == '/' && col + 1 < currentLine.length() && currentLine[col+1] == '/') {
            col = currentLine.length(); // 跳过本行
            continue;
        }
        // 2. 多行注释 /* ... */
        if (ch == '/' && col + 1 < currentLine.length() && currentLine[col+1] == '*') {
            col += 2;
            bool closed = false;
            while(true) {
                if (col >= currentLine.length()) {
                    if (!getline(file, currentLine)) return {END_OFF, "", lineNo};
                    currentLine += '\n';
                    lineNo++;
                    col = 0;
                }
                if (col + 1 < currentLine.length() && currentLine[col] == '*' && currentLine[col+1] == '/') {
                    col += 2;
                    closed = true;
                    break;
                }
                col++;
            }
            if(closed) continue;
        }

        // 1. 标识符和关键字
        if (isalpha(ch) || ch == '_') {
            buffer += ch;
            col++;
            while (col < currentLine.length()) {
                char c = currentLine[col];
                if (isalnum(c) || c == '_') {
                    buffer += c;
                    col++;
                } else break;
            }
            
            static std::unordered_map<string, TokenType> kwMap = {
                {"int", KW_INT}, {"void", KW_VOID}, {"return", KW_RETURN},
                {"if", KW_IF}, {"else", KW_ELSE}, {"float", KW_FLOAT},
                {"const", KW_CONST}, {"main", KW_MAIN} 
            };
            if (kwMap.count(buffer)) return {kwMap[buffer], buffer, lineNo};
            return {ID, buffer, lineNo};
        }

        // 2. 数字 (支持浮点)
        if (isdigit(ch)) {
            buffer += ch;
            col++;
            bool isFloat = false;
            while (col < currentLine.length()) {
                char c = currentLine[col];
                if (isdigit(c)) {
                    buffer += c;
                    col++;
                } else if (c == '.') {
                    if (isFloat) break;
                    isFloat = true;
                    buffer += c;
                    col++;
                } else {
                    break;
                }
            }
            return {isFloat ? FLOAT_CONST : INT_CONST, buffer, lineNo};
        }

        // 3. 符号
        col++; 
        
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

        buffer = ch;
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