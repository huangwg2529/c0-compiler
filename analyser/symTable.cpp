#include <iostream>
#include "analyser/symTable.h"
#include "analyser/symbol.h"

namespace cc0 {

    std::vector<Symbol> SymTable::getFunc() {
        std::vector<Symbol> res;
        for(auto& it: _symbols) {
            if (it.isFunction())
                res.emplace_back(it);
        }
        return res;
    }

    int32_t SymTable::getFuncSize() {
        int32_t order = 0;
        for(int i=0; i<_next_index; i++) {
            if(_symbols[i].isFunction())
                order++;
        }
        return order;
    }

    SymType SymTable::getFuncParamType(int index) {
        return _symbols[index].getType();
    }

    void SymTable::addVar(std::string name, bool isConst, SymType type) {
        _symbols.emplace_back(new Symbol(name, false, isConst, type, _next_index, 0));
        _next_index++;
    }

    int SymTable::getVarIndex(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return _symbols[i].getIndex();
        }
    }

    int32_t SymTable::addFunc(std::string name, SymType type) {
        _symbols.emplace_back(new Symbol(name, true, false, type, _next_index, 0));
        return _next_index++;
    }

    bool SymTable::isDeclared(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return true;
        }
        return false;
    }

    bool SymTable::isFunction(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName()) {
                if(_symbols[i].isFunction())
                    return true;
                else
                    return false;
            }
        }
        return false;
    }

    bool SymTable::isMainExisted() {
        for(int i=0; i<_next_index; i++) {
            if(_symbols[i].getName() == "main")
                return true;
        }
        return false;
    }

    bool SymTable::isConstantExisted(cc0::SymType type, std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(_symbols[i].getName() == name && _symbols[i].getType() == type)
                return true;
        }
        return false;
    }

    SymType SymTable::getType(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return _symbols[i].getType();
        }
    }

    bool SymTable::isConst(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return _symbols[i].isConst();
        }
    }

    SymType SymTable::getFuncType(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName() && _symbols[i].isFunction())
                return _symbols[i].getType();
        }
    }

    int32_t SymTable::getFuncParamNum(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName() && _symbols[i].isFunction())
                return _symbols[i].getParamNum();
        }
        return -1;
    }

    void SymTable::setFuncParamNum(std::string name, int32_t param_num) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName() && _symbols[i].isFunction()) {
                _symbols[i].setParamNum(param_num);
                return;
            }
        }
    }

    int32_t SymTable::getIndex(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return _symbols[i].getIndex();
        }
    }

    int32_t SymTable::getFuncOrder(std::string name) {
        int32_t order = 0;
        for(int i=0; i<_next_index; i++) {
            if(_symbols[i].isFunction()) {
                if(name == _symbols[i].getName())
                    break;
                order++;
            }
        }
        return order;
    }

    std::string SymTable::getNameByIndex(int32_t index) {
        return _symbols[index].getName();
    }

    bool SymTable::isInit(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName())
                return _symbols[i].isInit();
        }
    }

    void SymTable::initVar(std::string name) {
        for(int i=0; i<_next_index; i++) {
            if(name == _symbols[i].getName()) {
                _symbols[i].initVar();
                return;
            }
        }
    }

    void SymTable::print() {
        std::cout << "size: " << _symbols.size() << std::endl;
        for(int i=0; i<_next_index; i++) {
            std::cout << i << " name=" << _symbols[i].getName() << ", isFunc=" << std::boolalpha << _symbols[i].isFunction() << ", type=" << _symbols[i].getType() << ", index=" << _symbols[i].getIndex() << ", isInit=" << std::boolalpha << _symbols[i].isInit() << ", params=" << _symbols[i].getParamNum() << std::endl;
        }
    }

}

