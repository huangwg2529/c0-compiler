#pragma once

#include <string>
#include <cstdint>
#include <utility>
#include <vector>

namespace cc0 {

    enum SymType {
        VOID_TYPE,      // 函数
        CHAR_TYPE,
        INT_TYPE,       // 函数、变量
        DOUBLE_TYPE,
        STRING_TYPE,    // 存储字符串字面量
        UNLIMITED       // 无限制，函数调用时判断
    };

    class Symbol final {
    private:
        using uint64_t = std::uint64_t;
        using int32_t = std::int32_t;

    public:
        Symbol(std::string name, bool isFunc, bool isConst, SymType type, int32_t index, int32_t param_num)
            :_name(std::move(name)), _isFunc(isFunc), _isConst(isConst), _type(type), _index(index), _param_num(param_num) {};
        Symbol(Symbol* t) { _name = t->_name; _isFunc = t->_isFunc; _isConst = t->_isConst; _type = t->_type; _index = t->_index; _param_num = t->_param_num; };

    public:
        std::string getName() { return _name; };
        bool isFunction() const { return _isFunc; };
        SymType getType() const { return _type; };
        bool isConst() const { return _isConst; };
        int32_t getIndex() const { return _index; };
        int32_t getParamNum() const { return _param_num; };
        bool isInit() const { return _isInit; };
        void initVar() { _isInit = true; };
        void setParamNum(int32_t param_num) { _param_num = param_num; };

    private:
        std::string _name;    // 标识符
        bool _isFunc;         // 1 为函数，0 为变量
        bool _isConst;
        SymType _type;
        int32_t _index;       // 在符号表的索引
        bool _isInit = false; // 变量是否初始化了
        int32_t _param_num;   // 函数的参数数量
    };
}