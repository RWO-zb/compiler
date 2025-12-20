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
        exit(1);
    }
    // 预读取第一行，方便处理
    if (!getline(file, currentLine)) {
        currentLine = "";
    } else {
        currentLine += '\n'; // 补全换行符，这对行注释很重要
    }
}

Lexer::~Lexer() {
    if (file.is_open()) file.close();
}

// 获取下一个字符，自动处理换行
char Lexer::getChar() {
    if (col >= currentLine.length()) {
        if (!getline(file, currentLine)) {
            return EOF; // 文件结束
        }
        currentLine += '\n';
        lineNo++;
        col = 0;
    }
    return currentLine[col++];
}

// 回退一个字符 (用于最大匹配原则)
void Lexer::retract() {
    if (col > 0) col--;
}

// 核心：基于 DFA 的 Token 解析
Token Lexer::nextInternal() {
    State state = START;
    string buffer;
    TokenType type;

    while (state != DONE) {
        char ch = getChar();
        
        // 如果文件结束且缓冲区为空，则结束
        if (ch == EOF) {
            if (buffer.empty()) return {END_OFF, "", lineNo};
            // 否则可能还在某个状态中，强行结束（例如文件末尾没有换行符的数字）
            // 这里简单处理：如果正在解析数字或ID，直接返回
            if (state == IN_INT) return {INT_CONST, buffer, lineNo};
            if (state == IN_ID) return {ID, buffer, lineNo}; // 需查表
            return {END_OFF, "", lineNo};
        }

        switch (state) {
        case START:
            if (isspace(ch)) {
                // 跳过空白，保持 START 状态
                continue; 
            }
            buffer += ch;
            if (isalpha(ch) || ch == '_') { state = IN_ID; }
            else if (isdigit(ch))         { state = IN_INT; }
            else if (ch == '.')           { state = IN_FLOAT_DOT; } // .123 形式暂不支持，这里指 1.23 的 .
            else if (ch == '=')           { state = IN_EQ; }
            else if (ch == '>')           { state = IN_GT; }
            else if (ch == '<')           { state = IN_LT; }
            else if (ch == '!')           { state = IN_NOT; }
            else if (ch == '&')           { state = IN_AND; }
            else if (ch == '|')           { state = IN_OR; }
            else if (ch == '/')           { state = IN_COMMENT_START; }
            else {
                // 单字符符号，直接返回
                state = DONE;
                switch (ch) {
                    case '+': type = OP_PLUS; break;
                    case '-': type = OP_MINUS; break;
                    case '*': type = OP_MUL; break;
                    case '%': type = OP_MOD; break;
                    case '(': type = SE_LPAREN; break;
                    case ')': type = SE_RPAREN; break;
                    case '{': type = SE_LBRACE; break;
                    case '}': type = SE_RBRACE; break;
                    case ';': type = SE_SEMICOLON; break;
                    case ',': type = SE_COMMA; break;
                    default: 
                        // 未知字符，简单处理为 END_OFF 或报错
                        std::cerr << "Unknown char: " << ch << " at line " << lineNo << std::endl;
                        return {END_OFF, buffer, lineNo}; 
                }
            }
            break;

        case IN_ID:
            if (isalnum(ch) || ch == '_') {
                buffer += ch; // 保持状态
            } else {
                retract(); // 读到了非标识符字符，回退
                state = DONE;
                // 查关键字表
                static std::unordered_map<string, TokenType> kwMap = {
                    {"int", KW_INT}, {"void", KW_VOID}, {"return", KW_RETURN},
                    {"if", KW_IF}, {"else", KW_ELSE}, {"float", KW_FLOAT},
                    {"const", KW_CONST}, {"main", KW_MAIN}, {"while", KW_WHILE}
                };
                if (kwMap.count(buffer)) type = kwMap[buffer];
                else type = ID;
            }
            break;

        case IN_INT:
            if (isdigit(ch)) {
                buffer += ch;
            } else if (ch == '.') {
                buffer += ch;
                state = IN_FLOAT_DOT;
            } else {
                retract();
                state = DONE;
                type = INT_CONST;
            }
            break;

        case IN_FLOAT_DOT:
            if (isdigit(ch)) {
                buffer += ch;
                state = IN_FLOAT;
            } else {
                // 123. 后面不是数字，如果是 C 语言 1. 是合法的 float
                // 这里假设必须有小数部分，或者回退。
                // 简单处理：作为 float 返回
                retract();
                state = DONE;
                type = FLOAT_CONST; 
            }
            break;

        case IN_FLOAT:
            if (isdigit(ch)) {
                buffer += ch;
            } else {
                retract();
                state = DONE;
                type = FLOAT_CONST;
            }
            break;

        case IN_EQ: // 已读 =
            if (ch == '=') { buffer += ch; type = OP_EQ; state = DONE; }
            else { retract(); type = OP_ASSIGN; state = DONE; }
            break;

        case IN_GT: // 已读 >
            if (ch == '=') { buffer += ch; type = OP_GE; state = DONE; }
            else { retract(); type = OP_GT; state = DONE; }
            break;

        case IN_LT: // 已读 <
            if (ch == '=') { buffer += ch; type = OP_LE; state = DONE; }
            else { retract(); type = OP_LT; state = DONE; }
            break;

        case IN_NOT: // 已读 !
            if (ch == '=') { buffer += ch; type = OP_NEQ; state = DONE; }
            else { retract(); type = OP_NOT; state = DONE; }
            break;

        case IN_AND: // 已读 &
            if (ch == '&') { buffer += ch; type = OP_AND; state = DONE; }
            else { 
                // 单个 & 非法 (C--通常没有位运算)
                 std::cerr << "Invalid char & at line " << lineNo << std::endl;
                 return {END_OFF, "", lineNo};
            }
            break;

        case IN_OR: // 已读 |
            if (ch == '|') { buffer += ch; type = OP_OR; state = DONE; }
            else {
                 std::cerr << "Invalid char | at line " << lineNo << std::endl;
                 return {END_OFF, "", lineNo};
            }
            break;

        case IN_COMMENT_START: // 已读 /
            if (ch == '/') { 
                state = IN_LINE_COMMENT; 
                buffer.clear(); // 注释内容不需要保留为 Token
            } else if (ch == '*') {
                state = IN_BLOCK_COMMENT;
                buffer.clear();
            } else {
                retract();
                type = OP_DIV;
                state = DONE;
            }
            break;

        case IN_LINE_COMMENT:
            if (ch == '\n') {
                state = START; // 注释结束，重新开始
            }
            // 否则一直消耗字符
            break;

        case IN_BLOCK_COMMENT:
            if (ch == '*') { state = IN_BLOCK_COMMENT_END; }
            // 否则一直消耗
            break;
            
        case IN_BLOCK_COMMENT_END:
            if (ch == '/') { state = START; } // */ 结束
            else if (ch == '*') { state = IN_BLOCK_COMMENT_END; } // **...
            else { state = IN_BLOCK_COMMENT; } // *a 继续注释
            break;

        case DONE:
            break;
        }
    }
    return {type, buffer, lineNo};
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