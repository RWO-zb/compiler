#pragma once
#include <map>
#include <vector>
#include <string>
#include "../../compiler_ir/include/Value.h" 

// [Role D] 负责人
class SymbolTable {
private:
    // 作用域栈：vector 的每个元素代表一层作用域
    // map: 变量名 -> LLVM Value* (由 Role C 在生成代码时填入)
    std::vector<std::map<std::string, Value*>> scopes;

public:
    SymbolTable() { enterScope(); } // 默认全局作用域

    void enterScope() {
        scopes.push_back({});
    }

    void exitScope() {
        if (!scopes.empty()) scopes.pop_back();
    }

    // 插入符号
    bool put(const std::string& name, Value* val) {
        if (scopes.back().count(name)) return false; // 重复定义
        scopes.back()[name] = val;
        return true;
    }

    // 查找符号 (从栈顶向下查找)
    Value* get(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) return (*it)[name];
        }
        return nullptr;
    }
    
    // [Role D] TODO: 可能需要辅助函数判断是否是全局作用域
    bool isGlobal() const { return scopes.size() == 1; }
};