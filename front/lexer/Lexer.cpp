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
        currentLine += '\n'; // 补全换行符
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
            // 强行结束残留状态
            if (state == IN_INT) return {INT_CONST, buffer, lineNo};
            if (state == IN_ID) {
                // [修复] EOF处的 ID 也要填表
                if (symTable) symTable->put(buffer, nullptr); 
                return {ID, buffer, lineNo};
            }
            return {END_OFF, "", lineNo};
        }

        switch (state) {
        case START:
            if (isspace(ch)) {
                continue; // 跳过空白
            }
            buffer += ch;
            if (isalpha(ch) || ch == '_') { state = IN_ID; }
            else if (isdigit(ch))         { state = IN_INT; }
            else if (ch == '.')           { state = IN_FLOAT_DOT; }
            else if (ch == '=')           { state = IN_EQ; }
            else if (ch == '>')           { state = IN_GT; }
            else if (ch == '<')           { state = IN_LT; }
            else if (ch == '!')           { state = IN_NOT; }
            else if (ch == '&')           { state = IN_AND; }
            else if (ch == '|')           { state = IN_OR; }
            else if (ch == '/')           { state = IN_COMMENT_START; }
            else {
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
                // [修改] 移除了 "while"
                static std::unordered_map<string, TokenType> kwMap = {
                    {"int", KW_INT}, {"void", KW_VOID}, {"return", KW_RETURN},
                    {"if", KW_IF}, {"else", KW_ELSE}, {"float", KW_FLOAT},
                    {"const", KW_CONST}, {"main", KW_MAIN}
                };
                
                if (kwMap.count(buffer)) {
                    type = kwMap[buffer];
                } else {
                    type = ID;
                    // [核心改进] 满足“词法分析器填写符号表”的要求
                    // 将识别到的标识符名称填入符号表，Value 暂时置空
                    // 这里的 put 会利用 SymbolTable 默认的全局作用域
                    if (symTable) {
                        symTable->put(buffer, nullptr);
                    }
                }
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
                retract();
                state = DONE;
                type = FLOAT_CONST; // 处理 1. 这种情况
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
                buffer.clear(); 
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
                state = START; 
            }
            break;

        case IN_BLOCK_COMMENT:
            if (ch == '*') { state = IN_BLOCK_COMMENT_END; }
            break;
            
        case IN_BLOCK_COMMENT_END:
            if (ch == '/') { state = START; } 
            else if (ch == '*') { state = IN_BLOCK_COMMENT_END; } 
            else { state = IN_BLOCK_COMMENT; } 
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