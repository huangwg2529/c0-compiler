#pragma once

#include <cstdint>
#include "symbol.h"

namespace cc0 {

    class SymTable final {
    private:
        using int32_t = std::int32_t;

    public:
        SymTable() {};

        // 返回函数表
        std::vector<Symbol> getFunc();
        std::vector<Symbol> getSymbols() { return _symbols; }

        // 获取函数数量
        int32_t getFuncSize();
        // 获取函数参数的数据类型, index 为第几个参数
        SymType getFuncParamType(int index);

        // 添加变量/常量
        void addVar(std::string name, bool isConst, SymType type);
        // 获取变量位置
        int getVarIndex(std::string name);
        // 添加定义的函数
        int32_t addFunc(std::string name, SymType type);
        // 标识符是否已存在
        bool isDeclared(std::string name);
        // 标识符是否为函数
        bool isFunction(std::string name);
        // 是否有 main 函数
        bool isMainExisted();
        // 字面量是否已存在
        bool isConstantExisted(SymType type, std::string name);
        // 获取符号类型
        bool isConst(std::string name);
        SymType getType(std::string name);
        SymType getFuncType(std::string name);
        // 获取函数参数数量
        int32_t getFuncParamNum(std::string name);
        // 修改参数数量
        void setFuncParamNum(std::string name, int32_t param_num);
        // 获取符号在符号表的位置
        int32_t getIndex(std::string name);
        // 无视变量，只看函数是第几个
        int32_t getFuncOrder(std::string name);
        // 获取函数名
        std::string getNameByIndex(int32_t index);
        // 初始化变量
        void initVar(std::string name);
        // 是否初始化
        bool isInit(std::string name);

        void print();


    private:
        std::vector<Symbol> _symbols;
        int32_t _next_index = 0;
    };
}
