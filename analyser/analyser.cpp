#include "analyser.h"

#include <climits>

namespace cc0 {
	std::pair<std::map<int32_t ,std::vector<Instruction>>, std::optional<CompilationError>> Analyser::Analyse() {
		auto err = analyseC0Program();
		if (err.has_value())
			return std::make_pair(std::map<int32_t ,std::vector<Instruction>>(), err);
		else
			return std::make_pair(_instructions, std::optional<CompilationError>());
	}

    // <C0-program> ::= {<variable-declaration>}{<function-definition>}
    // variable: ['const'] <type-specifier> <identifier> [ '=' <expr> ] { ',' <init-declarator> }
    // function: <type-specifier> <identifier> '(' [list] ')'
    // 两条文法在无 const 的情况下有相同的 First V_t：<type-specifier> <identifier>
    // 为了控制两个的顺序和处理 const，这里还是直接回溯吧
	std::optional<CompilationError> Analyser::analyseC0Program() {
	    // 有个参数表明是哪个函数的局部变量声明
	    // 这里是全局变量，用 -1 表示
	    auto err = analyseVariableDeclaration(-1);
	    if(err.has_value())
            return err;

	    err = analyseFunctionDefinition();
	    if(err.has_value())
	        return err;

	    // printSym();

	    // 检查有没有 main 函数
	    if(!isMainExisted())
	        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedMain);

	    return {};
	}

	// {<variable-declaration>}
	// <variable-declaration> ::= [<const-qualifier>]<type-specifier><init-declarator-list>';'
    // <type-specifier>         ::= <simple-type-specifier>
    // <simple-type-specifier>  ::= 'void'|'int'|'char'|'double'
    // <const-qualifier>        ::= 'const'
    // 变量的类型，不能是void或const void
    // const修饰的变量必须被显式初始化
	std::optional<CompilationError> Analyser::analyseVariableDeclaration(int32_t funcIndex) {
        while(true) {
            // 预读，可能不是 variable-declaration
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType() != TokenType::CONST &&
               next.value().GetType() != TokenType::VOID &&
               next.value().GetType() != TokenType::INT &&
               next.value().GetType() != TokenType::CHAR &&
               next.value().GetType() != TokenType::DOUBLE) {
                unreadToken();
                return {};
            }

            bool isConst = false;
            if(next.value().GetType() == TokenType::CONST) {
                isConst = true;
                next = nextToken();
                if(!next.has_value()) // const 后面就没了
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);
            }

            if(next.value().GetType() == TokenType::VOID) {
                if(isConst) // 说明这里是 const void
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrVariableVoid);
                // 说明读入的是 void ，回溯并跳转到函数继续处理
                unreadToken();
                return {};
            }

            SymType type;
            switch(next.value().GetType()) {
                case INT:
                    type = INT_TYPE;
                    break;
                case CHAR:
                    type = CHAR_TYPE;
                    break;
                case DOUBLE:
                    type = DOUBLE_TYPE;
                    break;
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);
            }

//            if(next.value().GetType() != TokenType::INT &&
//               next.value().GetType() != TokenType::CHAR &&
//               next.value().GetType() != TokenType::DOUBLE)
//                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);

            // 这里必然是 int/char/double 或 const int/char/double，由 isConst 和 type 判断

            // 如果没有 const，接下来先预读两个，如果是 <identifier> '('，说明需要跳转到函数定义
            if(!isConst) {
                auto pre_next = nextToken();
                if(!pre_next.has_value() || pre_next.value().GetType() != TokenType::IDENTIFIER)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                // 全局下 main 必须是函数，强制跳转到函数处理
                if(funcIndex == -1 && pre_next.value().GetValueString() == "main") {
                    unreadToken();  // 回溯 int main
                    unreadToken();
                    return {};
                }
                auto pre_next2 = nextToken();
                // 把这两次预读都回退
                unreadToken();
                unreadToken();
                if(!pre_next2.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
                if(pre_next2.value().GetType() == TokenType::LEFT_BRACKET) {
                    // 这里还需要回退 int，彻底回溯完
                    unreadToken();
                    return {};
                }
            }

            // <init-declarator-list>
            auto err = analyseInitDeclaratorList(funcIndex, isConst, type);
            if(err.has_value())
                return err;

            // ;
            auto sem = nextToken();
            if(!sem.has_value() || sem.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        }

        return {};
	}

    // <init-declarator-list> ::= <init-declarator>{','<init-declarator>}
    std::optional<CompilationError> Analyser::analyseInitDeclaratorList(int32_t funcIndex, bool isConst, SymType& type) {
        auto err = analyseInitDeclarator(funcIndex, isConst, type);
        if(err.has_value())
            return err;

        // 预读 ,
        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType() != TokenType::COMMA_SIGN) {
                unreadToken();
                return {};
            }

            auto err = analyseInitDeclarator(funcIndex, isConst, type);
            if(err.has_value())
                return err;
        }
	}

    // <init-declarator> ::= <identifier>[<initializer>]
    // <initializer> ::= '='<expression>
    std::optional<CompilationError> Analyser::analyseInitDeclarator(int32_t funcIndex, bool isConst, SymType& type) {
	    // identifier
	    auto ident = nextToken();
	    if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

	    // 查符号表看看是否已声明，再添加
        // 全局变量：需要查全局变量表，但不需要查函数表，因为函数还没开始定义
        // 局部变量：只需查找局部变量表
        if(isDeclared(funcIndex, ident.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

        addVar(funcIndex, ident.value().GetValueString(), isConst, type);

	    // 预读 =
	    // const 必须显式初始化，变量随意
	    auto next = nextToken();
	    if(!next.has_value()) {
            if(isConst) // const 必须显式初始化
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
	        return {};
	    }
	    if(next.value().GetType() != TokenType::ASSIGN_SIGN) {
            if(isConst) // const 必须显式初始化
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);

            // 没有初始化，局部变量在栈上先为它分配内存
	        // 全局变量未初始化直接默认为 0
            if(funcIndex != -1) {
	            _instructions[funcIndex].emplace_back(Operation::SNEW, 1);
	        } else {
                _instructions[funcIndex].emplace_back(Operation::IPUSH, 0);
                initVar(funcIndex, ident.value().GetValueString());
            }

            unreadToken();
            return {};
	    }

	    // std::cout << "declare var: name = " << ident.value().GetValueString() << "; type = " << type << std::endl;

        // expression
        SymType secType;
	    auto err = analyseExpression(secType, funcIndex);
	    if(err.has_value())
            return err;

        // std::cout << "declare var: expression type = " << secType << std::endl;

	    switch(type) {
	        case INT_TYPE: {
	            if(secType == DOUBLE_TYPE)
	                _instructions[funcIndex].emplace_back(Operation::D2I);
	            break;
	        }
	        case CHAR_TYPE: {
	            if(secType == DOUBLE_TYPE) {
	                _instructions[funcIndex].emplace_back(Operation::D2I);
	                _instructions[funcIndex].emplace_back(Operation::I2C);
	            }
	            else if(secType == INT_TYPE)
	                _instructions[funcIndex].emplace_back(Operation::I2C);
	            break;
	        }
	        case DOUBLE_TYPE: {
	            if(secType != DOUBLE_TYPE)
	                _instructions[funcIndex].emplace_back(Operation::I2D);
	            break;
	        }
            default:
                break;
	    }

	    // 设为已初始化
	    initVar(funcIndex, ident.value().GetValueString());

	    // 对于变量声明而言，表达式计算出来的值存放在栈顶就是变量的值了，以后加载就加载这个地方的值

        return {};
	}

	// <function-definition> ::= <type-specifier><identifier><parameter-clause><compound-statement>
    // <parameter-clause> ::= '(' [<parameter-declaration-list>] ')'
    std::optional<CompilationError> Analyser::analyseFunctionDefinition() {
	    while(true) {
	        // <type-specifier>
            // 如果读完了，就直接退出。有的话必须是 type
            auto type = nextToken();
            if (!type.has_value())
                return {};
            SymType symType;
            switch(type.value().GetType()) {
                case VOID:
                    symType = VOID_TYPE;
                    break;
                case CHAR:
                    symType = CHAR_TYPE;
                    break;
                case INT:
                    symType = INT_TYPE;
                    break;
                case DOUBLE:
                    symType = DOUBLE_TYPE;
                    break;
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);
            }

            // <identifier>
            auto ident = nextToken();
            if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            // 查符号表
            // 查全局变量表是否重名，查常量表是否有函数重名
            if(isDeclared(-1, ident.value().GetValueString()) || isDeclaredFunc(ident.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode ::ErrDuplicateDeclaration);
            // 参数数量在确定参数后修改
            int32_t param_num = 0;
            // 添加符号表
            int32_t funcIndex = addFunc(ident.value().GetValueString(), symType);

            // <parameter-clause> ::= '(' [<parameter-declaration-list>] ')'
            // '('
            auto next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionDefinition);

            // 预读，看看有没有参数 const / int
            next = nextToken();
            unreadToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionDefinition);
            if(next.value().GetType() == TokenType::CONST || next.value().GetType() == TokenType::INT) {
                // <parameter-declaration-list>
                auto err = analyseParameterDeclarationList(funcIndex, param_num);
                if(err.has_value())
                    return err;
            }

            // 修改参数数量
            setFuncParamNum(ident.value().GetValueString(), param_num);

            // ')'
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionDefinition);

            // <compound-statement>
            auto err = analyseCompoundStatement(funcIndex);
            if(err.has_value())
                return err;
        }
	}

    // <parameter-declaration-list> ::= <parameter-declaration>{','<parameter-declaration>}
    std::optional<CompilationError> Analyser::analyseParameterDeclarationList(int32_t funcIndex, int32_t& param_num) {
        // <parameter-declaration>
	    auto err = analyseParameterDeclaration(funcIndex);
	    if(err.has_value())
            return err;
	    // 参数数量 +1
        param_num++;

	    // {','<parameter-declaration>}
	    while(true) {
	        auto next = nextToken();
	        if(!next.has_value())
                return {};
	        if(next.value().GetType() != TokenType::COMMA_SIGN) {
	            unreadToken();
                return {};
	        }

	        err = analyseParameterDeclaration(funcIndex);
	        if(err.has_value())
                return err;
            // 参数数量 +1
            param_num++;
	    }
	}

    // <parameter-declaration> ::= [<const-qualifier>]<type-specifier><identifier>
    // const int / int
    std::optional<CompilationError> Analyser::analyseParameterDeclaration(int32_t funcIndex) {
	    auto next = nextToken();
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);

	    // [<const-qualifier>]
	    bool isConst = false;
	    if(next.value().GetType() == TokenType::CONST) {
	        isConst = true;
	        next = nextToken();
	        if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);
	    }

        SymType type;
	    switch (next.value().GetType()) {
            case INT:
                type = INT_TYPE;
                break;
            case CHAR:
                type = CHAR_TYPE;
                break;
            case DOUBLE:
                type = DOUBLE_TYPE;
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedType);
	    }

        // <identifier>
        auto ident = nextToken();
        if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        // 添加参数到局部符号表
        addVar(funcIndex, ident.value().GetValueString(), isConst, type);
        // 参数必然可以看作已初始化的
        initVar(funcIndex, ident.value().GetValueString());

        return {};
	}


    // <function-call> ::= <identifier> '(' [<expression-list>] ')'
    // 判断参数数量和类型
    std::optional<CompilationError> Analyser::analyseFunctionCall(SymType& type, int32_t funcIndex) {
        // <identifier>
        auto ident = nextToken();
        if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        // https://forum.lazymio.cn/t/287
        // 按照助教说法，函数内如果存在同名的变量，会屏蔽外层的函数定义，所以无法递归
        if(isDeclared(funcIndex, ident.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidStatementSeq);
        // 获取参数数量，如果是-1则没有定义这个函数
        int32_t param_num = getFuncParamNum(ident.value().GetValueString());
        if(param_num == -1)
            return std::make_optional<CompilationError>(_current_pos, ErrCallUndefined);
        // 设置类型为函数的返回值类型
        type = getFuncType(ident.value().GetValueString());
//        // 判断函数返回值，如果是在表达式中参与运算的话返回值必须为int
//        if(type == SymType::CONST_INT && getFuncType(ident.value().GetValueString()) != INT_TYPE)
//            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

        // '('
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

        // [<expression-list>]
        // 这里需要把参数都压栈了
        // 还需查表找到参数类型
        if(param_num > 0) {
            auto err = analyseExpressionList(funcIndex, getConstantIndex(ident.value().GetValueString()), param_num);
            if(err.has_value())
                return err;
        }

        // ')'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

        // 获取函数在函数表的位置
        int32_t order = getFuncOrder(ident.value().GetValueString());
        // 添加函数调用的指令
        _instructions[funcIndex].emplace_back(Operation::CALL, order);

        return {};
	}

    // <expression-list> ::= <expression>{','<expression>}
    // 函数调用的传参数量以及每一个参数的数据类型（不考虑const），都必须和函数声明中的完全一致
    // 第一个参数是指当前在哪个函数体内，第二个参数是指被调用的函数是哪个
    std::optional<CompilationError> Analyser::analyseExpressionList(int32_t funcIndex, int32_t constIndex, int32_t param_num) {
	    int paramNum = param_num;
	    // <expression>
        // 这是第一个参数，查符号表找第一项的type
        SymType secType;
        auto err = analyseExpression(secType, funcIndex);
        if(err.has_value())
            return err;

        // std::cout << "-------------------param type = " << secType << std::endl;

        SymType firstType = getFuncParamType(constIndex, param_num-paramNum);

        // std::cout << "??????????????????   funcIndex = " << funcIndex << "  freq = " << param_num-paramNum << std::endl;

        // printSym();

        // std::cout << "--------------need type = " << firstType << std::endl;

        // 将右侧表达式隐式转换为左侧标识符的类型
        if(firstType == SymType::INT_TYPE) {
            if(secType == SymType::DOUBLE_TYPE) {
                // 把表达式的值转换为 int
                _instructions[funcIndex].emplace_back(Operation::D2I);
            }
        }
        else if(firstType == SymType::CHAR_TYPE) {
            if(secType == SymType::DOUBLE_TYPE) {
                _instructions[funcIndex].emplace_back(Operation::D2I);
                _instructions[funcIndex].emplace_back(Operation::I2C);
            }
            else if(secType == SymType::INT_TYPE) {
                _instructions[funcIndex].emplace_back(Operation::I2C);
            }
        }
        else if(firstType == SymType::DOUBLE_TYPE) {
            if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                // 把表达式的值转换为 double
                _instructions[funcIndex].emplace_back(Operation::I2D);
        }

        paramNum--;

        // {','<expression>}
        while(paramNum) {
            auto next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::COMMA_SIGN)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

            SymType secType;
            err = analyseExpression(secType, funcIndex);
            if(err.has_value())
                return err;

            SymType firstType = getFuncParamType(constIndex, param_num-paramNum);

            // 将右侧表达式隐式转换为左侧标识符的类型
            if(firstType == SymType::INT_TYPE) {
                if(secType == SymType::DOUBLE_TYPE) {
                    // 把表达式的值转换为 int
                    _instructions[funcIndex].emplace_back(Operation::D2I);
                }
            }
            else if(firstType == SymType::CHAR_TYPE) {
                if(secType == SymType::DOUBLE_TYPE) {
                    _instructions[funcIndex].emplace_back(Operation::D2I);
                    _instructions[funcIndex].emplace_back(Operation::I2C);
                }
                else if(secType == SymType::INT_TYPE) {
                    _instructions[funcIndex].emplace_back(Operation::I2C);
                }
            }
            else if(firstType == SymType::DOUBLE_TYPE) {
                if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                    // 把表达式的值转换为 double
                    _instructions[funcIndex].emplace_back(Operation::I2D);
            }

            paramNum--;
        }

        return {};
	}

	// <compound-statement> ::= '{' {<variable-declaration>} <statement-seq> '}'
    std::optional<CompilationError> Analyser::analyseCompoundStatement(int32_t funcIndex) {
	    bool isReturn = false;
	    // '{'
	    auto next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidCompoundStatement);

        // {<variable-declaration>}
	    auto err = analyseVariableDeclaration(funcIndex);
	    if(err.has_value())
            return err;

	    // <statement-seq>
	    err = analyseStatementSeq(funcIndex, isReturn);
	    if(err.has_value())
            return err;

	    // '}'
	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidCompoundStatement);

	    // 没写返回语句自动加返回指令
	    if(!isReturn) {
	        std::string name = getFuncName(funcIndex);
	        SymType type = getFuncType(name);
            switch(type) {
                case CHAR_TYPE:
                case INT_TYPE:
                    _instructions[funcIndex].emplace_back(Operation::IPUSH, 0);
                    _instructions[funcIndex].emplace_back(Operation::IRET);
                    break;
                case DOUBLE_TYPE:
                    _instructions[funcIndex].emplace_back(Operation::IPUSH, 0);
                    _instructions[funcIndex].emplace_back(Operation::IPUSH, 0);
                    _instructions[funcIndex].emplace_back(Operation::DRET);
                    break;
                default:
                    _instructions[funcIndex].emplace_back(Operation::RET);
                    break;
            }
        }

        return {};
	}

    // <statement-seq> ::= {<statement>}
    std::optional<CompilationError>Analyser::analyseStatementSeq(int32_t funcIndex, bool& isReturn) {
        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};
            unreadToken();
            switch(next.value().GetType()) {
                case LEFT_BRACE:
                case IF:
                case WHILE:
                case RETURN:
                case PRINT:
                case SCAN:
                case IDENTIFIER:
                case SEMICOLON: {
                    auto err = analyseStatement(funcIndex, isReturn);
                    if(err.has_value())
                        return err;
                    break;
                }
                default: {
                    return {};
                }
            }
        }
	}

    // <statement> ::= '{' <statement-seq> '}' | <condition-statement> | <loop-statement> | <jump-statement>
    //                  | <print-statement> | <scan-statement> | <assignment-expression>';' | <function-call>';' |';'
    // <jump-statement> ::= <return-statement>
    // <return-statement> ::= 'return' [<expression>] ';'
    // <condition-statement> ::= 'if' '(' <condition> ')' <statement> ['else' <statement>]
    // <loop-statement> ::= 'while' '(' <condition> ')' <statement>
    // <scan-statement>  ::= 'scan' '(' <identifier> ')' ';'
    // <print-statement> ::= 'print' '(' [<printable-list>] ')' ';'
    // <printable-list>  ::= <printable> {',' <printable>}
    // <printable> ::= <expression>
    std::optional<CompilationError>Analyser::analyseStatement(int32_t funcIndex, bool& isReturn) {
        auto next = nextToken();
        unreadToken();
        if(!next.has_value())
            return {};
        switch(next.value().GetType()) {
            case LEFT_BRACE: { // '{' <statement-seq> '}'
                // '{'
                nextToken();
                // <statement-seq>
                auto err = analyseStatementSeq(funcIndex, isReturn);
                if(err.has_value())
                    return err;
                // '}'
                auto next = nextToken();
                if(!next.has_value() || next.value().GetType() != RIGHT_BRACE)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidStatementSeq);
                break;
            }
            case IF: { // <condition-statement>
                auto err = analyseConditionStatement(funcIndex, isReturn);
                if(err.has_value())
                    return err;
                // std::cout << "condition: isReturn? " << isReturn << std::boolalpha << std::endl;
                break;
            }
            case WHILE: { // <loop-statement>
                auto err = analyseLoopStatement(funcIndex, isReturn);
                if(err.has_value())
                    return err;
                // std::cout << "loop: isReturn? " << isReturn << std::boolalpha << std::endl;
                break;
            }
            case RETURN: { // <jump-statement> ::= <return-statement>
                auto err = analyseJumpStatement(funcIndex);
                if(err.has_value())
                    return err;
                isReturn = true;
                break;
            }
            case PRINT: { // <print-statement>
                auto err = analysePrintStatement(funcIndex);
                if(err.has_value())
                    return err;
                break;
            }
            case SCAN: { // <scan-statement>
                auto err = analyseScanStatement(funcIndex);
                if(err.has_value())
                    return err;
                break;
            }
            case IDENTIFIER: {
                // <function-call>';'
                // <assignment-expression>';'

                // 预读
                nextToken();
                auto pre_next = nextToken();
                unreadToken();
                unreadToken();
                if(!pre_next.has_value() ||
                   (pre_next.value().GetType() != TokenType::ASSIGN_SIGN &&
                    pre_next.value().GetType() != TokenType::LEFT_BRACKET)
                    )
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidStatementSeq);
                if(pre_next.value().GetType() == ASSIGN_SIGN) {
                    auto err = analyseAssignmentExpression(funcIndex);
                    if(err.has_value())
                        return err;
                } else {
                    // <function-call>
                    // 这里函数调用不参与运算，返回值是啥都行
                    SymType unlimited;
                    auto err = analyseFunctionCall(unlimited, funcIndex);
                    if(err.has_value())
                        return err;
                    // 如果调用者不需要返回值，执行 pop 系列指令清除调用者栈帧得到的返回值
                    if(getFuncType(next.value().GetValueString()) == SymType::INT_TYPE)
                        _instructions[funcIndex].emplace_back(Operation::POP);
                }

                // ';'
                auto next = nextToken();
                if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

                break;
            }
            case SEMICOLON: { // ';'
                nextToken();
                break;
            }
            default:
                break;
        }

        return {};
	}

    // <condition-statement> ::= 'if' '(' <condition> ')' <statement> ['else' <statement>]
    // 必须要 if 和 else 都有 return 语句才是 isReturn = true
    std::optional<CompilationError>Analyser::analyseConditionStatement(int32_t funcIndex, bool& isReturn) {
	    TokenType type = TokenType::NULL_TOKEN;
	    // 'if'
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::IF)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        // '('
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        // <condition>
        // 如果 <condition> ::= <expression> == 0，false，否则为 true
        auto err = analyseCondition(type, funcIndex);
        if(err.has_value())
            return err;

        // 不满足条件就跳转
        Operation opt;
        switch(type) {
            case NULL_TOKEN:  // if(a)，栈顶 value == a
                opt = Operation::JE; // je：value 是 0 就跳转
                break;
            case LESS_SIGN:  // if(a < b)，栈顶 value ==
                opt = Operation::JGE;
                break;
            case LESS_EQUAL_SIGN:
                opt = Operation::JG;
                break;
            case GREATER_SIGN:
                opt = Operation::JLE;
                break;
            case GREATER_EQUAL_SIGN:
                opt = Operation::JL;
                break;
            case EQUAL_SIGN:
                opt = Operation::JNE;
                break;
            case NONEQUAL_SIGN:
                opt = Operation::JE;
                break;
            default:
                break;
        }
        int32_t tmp = _instructions[funcIndex].size();
        _instructions[funcIndex].emplace_back(opt);

        // ')'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        bool ifReturn = false;
        // <statement>
        err = analyseStatement(funcIndex, ifReturn);
        if(err.has_value())
            return err;

        // ['else' <statement>]
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() != TokenType::ELSE) {
            unreadToken();
            // 没有 else
            // 设置跳转指令的位置为这里
            _instructions[funcIndex][tmp].setX(_instructions[funcIndex].size());
            isReturn = false;
            return {};
        }
        // 有 else 的话
        // 执行完后需要跳过 else 的内容，即跳转到后面
        auto jmp = _instructions[funcIndex].size();
        _instructions[funcIndex].emplace_back(Operation::JMP);

        // 设置跳转指令的位置为这里
        _instructions[funcIndex][tmp].setX(_instructions[funcIndex].size());

        // <statement>
        bool elseReturn = false;
        err = analyseStatement(funcIndex, elseReturn);
        if(err.has_value())
            return err;

        // 设置跳转指令的位置为这里
        _instructions[funcIndex][jmp].setX(_instructions[funcIndex].size());

        isReturn = ifReturn && elseReturn;

        return {};
	}

    // <loop-statement> ::= 'while' '(' <condition> ')' <statement>
    // 运行过程：
    //           goto condition
    //      loop:
    //           statement
    // condition:
    //           if(condition)
    //                 goto loop
    // 所以这里要把 <condition> 生成的判断指令放在后面
    std::optional<CompilationError>Analyser::analyseLoopStatement(int32_t funcIndex, bool& isReturn) {
	    // 'while'
	    auto next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::WHILE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLoopStatement);

        // '('
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLoopStatement);

        auto i = _instructions[funcIndex].size();

        // <condition>
        TokenType type = TokenType::NULL_TOKEN;
        auto err = analyseCondition(type, funcIndex);
        if(err.has_value())
            return err;

        auto now = _instructions[funcIndex].size();
        int times = now - i;
        // 将 <condition> 生成的指令先挪出来
        std::vector<Instruction> conditions; // 备份到这
        for(;i!=now; i++) {
            conditions.emplace_back(_instructions[funcIndex][i]);
        }
        while(times--)
            _instructions[funcIndex].pop_back();

        // 生成一个跳转指令，循环开始之前先跳转到后面进行判断
        auto tmp = _instructions[funcIndex].size();
        _instructions[funcIndex].emplace_back(Operation::JMP);

        // ')'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLoopStatement);

        // 标记循环体的开始位置
        auto begin = _instructions[funcIndex].size();

        // <statement>
        err = analyseStatement(funcIndex, isReturn);
        if(err.has_value())
            return err;

        // 设置上述 jmp 指令的 offset
        auto order = _instructions[funcIndex].size();
        _instructions[funcIndex][tmp].setX(_instructions[funcIndex].size());

        // 把条件判断指令放在这里
        for(auto& it : conditions) {
            _instructions[funcIndex].emplace_back(std::move(it));
        }

        // 设置跳转指令，满足条件就跳转到循环体开始位置
        Operation opt;
        switch(type) {
            case NULL_TOKEN:  // if(a)，栈顶 value == a
                opt = Operation::JNE; // je：value 是 0 就跳转
                break;
            case LESS_SIGN:  // if(a < b)，栈顶 value ==
                opt = Operation::JL;
                break;
            case LESS_EQUAL_SIGN:
                opt = Operation::JLE;
                break;
            case GREATER_SIGN:
                opt = Operation::JG;
                break;
            case GREATER_EQUAL_SIGN:
                opt = Operation::JGE;
                break;
            case EQUAL_SIGN:
                opt = Operation::JE;
                break;
            case NONEQUAL_SIGN:
                opt = Operation::JNE;
                break;
            default:
                break;
        }
        _instructions[funcIndex].emplace_back(opt, begin);

        return {};
    }

    // <jump-statement> ::= <return-statement> ::= 'return' [<expression>] ';'
    std::optional<CompilationError>Analyser::analyseJumpStatement(int32_t funcIndex) {
	    // 'return'
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RETURN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidReturnStatement);

        // 查表看看有没有返回值
        bool ret_flag = false;
        std::string funcName = getFuncName(funcIndex);
        SymType funcType = getFuncType(funcName);
        if(funcType != SymType::VOID_TYPE) {
            // [<expression>]
            SymType secType;
            auto err = analyseExpression(secType, funcIndex);
            if(err.has_value())
                return err;
            ret_flag = true;
            // 如果函数return语句的表达式类型和函数声明的返回值类型不一致，应当对该表达式进行隐式类型转换后再返回
            if(funcType == SymType::INT_TYPE) {
                if(secType == SymType::DOUBLE_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::D2I);
            }
            else if(funcType == SymType::CHAR_TYPE) {
                if(secType == SymType::DOUBLE_TYPE) {
                    _instructions[funcIndex].emplace_back(Operation::D2I);
                    _instructions[funcIndex].emplace_back(Operation::I2C);
                }
                else if(secType == SymType::INT_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::I2C);
            }
            else if(funcType == SymType::DOUBLE_TYPE) {
                if(secType == SymType::INT_TYPE || secType ==SymType::CHAR_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::I2D);
            }
        }

        // ';'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        // 添加 iret 指令
        if(funcType == INT_TYPE || funcType == CHAR_TYPE)
            _instructions[funcIndex].emplace_back(Operation::IRET);
        else if(funcType == VOID_TYPE)
            _instructions[funcIndex].emplace_back(Operation::RET);
        else if(funcType == DOUBLE_TYPE)
            _instructions[funcIndex].emplace_back(Operation::DRET);
        return {};
	}

    // <print-statement> ::= 'print' '(' [<printable-list>] ')' ';'
    std::optional<CompilationError> Analyser::analysePrintStatement(int32_t funcIndex) {
	    // 'print'
	    auto next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::PRINT)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrintStatement);

        // '('
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrintStatement);

        // [<printable-list>]
        // 预读两个，如果遇到分号就没有它了，否则是有的
        int flag = true;
        auto pre_next = nextToken();
        auto pre_next_2 = nextToken();
        unreadToken();
        unreadToken();
        if(pre_next.has_value() && pre_next.value().GetType() == TokenType::RIGHT_BRACKET &&
           pre_next_2.has_value() && pre_next_2.value().GetType() == TokenType::SEMICOLON)
            flag = false;
        if(flag) {
            // <printable-list>
            auto err = analysePrintableList(funcIndex);
            if(err.has_value())
                return err;
        }

        // ')'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrintStatement);

        // ';'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        // 生成指令：输出换行
        _instructions[funcIndex].emplace_back(Operation::PRINTL);

        return {};
	}

    // <printable-list>  ::= <printable> {',' <printable>}
    std::optional<CompilationError> Analyser::analysePrintableList(int32_t funcIndex) {
	    // <printable>
        auto err = analysePrintable(funcIndex);
        if(err.has_value())
            return err;

        // {',' <printable>}
        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType() != TokenType::COMMA_SIGN) {
                unreadToken();
                return {};
            }

            // 生成指令：输出空格
            // 每个 <printable> 之间一个空格
            _instructions[funcIndex].emplace_back(Operation::BIPUSH, 32);
            _instructions[funcIndex].emplace_back(Operation::CPRINT);

            // <printable>
            err = analysePrintable(funcIndex);
            if(err.has_value())
                return err;
        }
    }

    // <printable> ::= <expression> | <string-literal> | <char-literal>
    std::optional<CompilationError> Analyser::analysePrintable(int32_t funcIndex) {
        // <printable> ::= <expression>
        // 这里不能是 void，也就是说可以是 int、const int、double（如果加了）
        auto next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrintStatement);
        if(next.value().GetType() == TokenType::CHAR_TOKEN) { // 字符字面量
            // 获取字符值，压栈
            auto str = next.value().GetValueString();
            const char *ch = str.c_str();
            // 将单字节值 byte 值提升至 int 值后入栈
            _instructions[funcIndex].emplace_back(Operation::BIPUSH, ch[0]);
            // 输出栈顶的值 ASCII 字符
            _instructions[funcIndex].emplace_back(Operation::CPRINT);
        }
        else if(next.value().GetType() == TokenType::STRING) { // 字符串字面量
            // 查表看看有没有一样的字面量
            if(!isConstantExisted(SymType::STRING_TYPE, next.value().GetValueString()))
                addConstant(next.value().GetValueString(), SymType::STRING_TYPE);
            // 获取字面量在常量表的位置
            auto index = getConstantIndex(next.value().GetValueString());
            // 生成指令
            // 加载常量表中字符串的地址值
            _instructions[funcIndex].emplace_back(Operation::LOADC, index);
            // 弹出栈顶的字符串地址，对每个 slot 的值进行输出
            _instructions[funcIndex].emplace_back(Operation::SPRINT);
        }
        else { // <expression>
            unreadToken();
            SymType type;
            auto err = analyseExpression(type, funcIndex);
            if(err.has_value())
                return err;

            // std::cout << "Printable: expression type = " << type << std::endl;

            // 生成指令：输出结果
            if(type == SymType::INT_TYPE)
                _instructions[funcIndex].emplace_back(Operation::IPRINT);
            else if(type == SymType::CHAR_TYPE)
                _instructions[funcIndex].emplace_back(Operation::CPRINT);
            else if(type == SymType::DOUBLE_TYPE)
                _instructions[funcIndex].emplace_back(Operation::DPRINT);
        }

        return {};
	}

    // <scan-statement> ::= 'scan' '(' <identifier> ')' ';'
    // scan的 <identifer> 必须是非 const 的变量，必须是可修改的
    std::optional<CompilationError>Analyser::analyseScanStatement(int32_t funcIndex) {
        // 'scan'
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SCAN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScanStatement);

        // '('
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScanStatement);

        // <identifier>
        auto ident = nextToken();
        if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        SymType type;
        // 是全局变量还是局部变量
        bool isGlobal = false;
        // 查符号表: 已声明、非const、局部符号表必然没有func，全局变量需要判断一下是不是函数
        if(!isDeclared(funcIndex, ident.value().GetValueString())) { // 局部符号表
            if(!isDeclared(-1, ident.value().GetValueString())) // 全局符号表
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
            // 说明是全局变量
            isGlobal = true;
            type = getVarType(-1, ident.value().GetValueString());
            if(isConst(funcIndex, ident.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
        } else {
            // 说明是局部变量
            type = getVarType(funcIndex, ident.value().GetValueString());
            if(isConst(funcIndex, ident.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
            // 如果没有初始化，这里就算初始化了
            initVar(funcIndex, ident.value().GetValueString());
        }

        // ')'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScanStatement);

        // ';'
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        // 生成指令：加载该变量的地址
        int16_t level_diff;
        int32_t offset;
        if(isGlobal) {
            offset = getVarIndex(-1, ident.value().GetValueString());
            level_diff = 1;
        } else { // 这里是局部变量，那么只能是局部变量在函数体内被调用了
            offset = getVarIndex(funcIndex, ident.value().GetValueString());
            level_diff = 0;
        }
        // 加载变量的地址
        _instructions[funcIndex].emplace_back(Operation::LOADA, level_diff, offset);

        if(type == SymType::DOUBLE_TYPE) {
            _instructions[funcIndex].emplace_back(Operation::DSCAN);
            _instructions[funcIndex].emplace_back(Operation::DSTORE);
        }
        else if(type == SymType::INT_TYPE) {
            // 从标准输入解析一个可有符号的十进制整数，将其转换为 int 得到 value，将 value 压入栈。
            _instructions[funcIndex].emplace_back(Operation::ISCAN);
            // 将获取的值存入该地址
            _instructions[funcIndex].emplace_back(Operation::ISTORE);
        }
        else if(type == SymType::CHAR_TYPE) {
            _instructions[funcIndex].emplace_back(Operation::CSCAN);
            _instructions[funcIndex].emplace_back(Operation::ISTORE);
        }


        return {};
	}



    // <assignment-expression> ::= <identifier><assignment-operator><expression>
    // <assignment-expression>没有值语义
    // <assignment-expression>左侧的标识符的值类型必须是可修改的变量（不能是const T、不能是函数名）
    // <assignment-expression>右侧的表达式必须是有值的（不能是void类型、不能是函数名）
    // 对于赋值表达式<assignment-expression>以及带有初始化的变量声明<init-declarator>
    // 如果=运算符两侧的类型不同，应该将右侧表达式隐式转换为左侧标识符的类型
    std::optional<CompilationError> Analyser::analyseAssignmentExpression(int32_t funcIndex) {
        // <identifier>
        auto ident = nextToken();
        if(!ident.has_value() || ident.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        SymType firstType;
        // 是全局变量还是局部变量
        bool isGlobal = false;
        // 查符号表: 已声明、非const、局部符号表必然没有func，全局变量需要判断一下是不是函数
        if(!isDeclared(funcIndex, ident.value().GetValueString())) { // 不是局部变量
            if(!isDeclared(-1, ident.value().GetValueString())) // 不是全局变量
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
            // 说明是全局变量
            isGlobal = true;
            firstType = getVarType(-1, ident.value().GetValueString());
            if(isConst(-1, ident.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
        } else {
            firstType = getVarType(funcIndex, ident.value().GetValueString());
            if(isConst(funcIndex, ident.value().GetValueString()))
               return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
        }

        // 生成指令：加载该变量的地址
        int16_t level_diff;
        int32_t offset;
        if(isGlobal) {
            offset = getVarIndex(-1, ident.value().GetValueString());
            level_diff = 1;
        } else { // 这里是局部变量，那么只能是局部变量在函数体内被调用了
            offset = getVarIndex(funcIndex, ident.value().GetValueString());
            level_diff = 0;
        }

        // 生成指令
        // 加载变量的地址
        _instructions[funcIndex].emplace_back(Operation::LOADA, level_diff, offset);

        // 如果没有初始化，这里就算初始化了
        if(isGlobal)
            initVar(-1, ident.value().GetValueString());
        else
            initVar(funcIndex, ident.value().GetValueString());

        // <assignment-operator>
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::ASSIGN_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);

        // <expression>
        SymType secType;
        auto err = analyseExpression(secType, funcIndex);
        if(err.has_value())
            return err;

        // 将右侧表达式隐式转换为左侧标识符的类型
        SymType type;
        if(firstType == SymType::INT_TYPE) {
            if(secType == SymType::INT_TYPE)
                type = SymType::INT_TYPE;
            else if(secType == CHAR_TYPE)
                type = SymType::INT_TYPE;
            else if(secType == SymType::DOUBLE_TYPE) {
                // 把表达式的值转换为 int
                _instructions[funcIndex].emplace_back(Operation::D2I);
                type = SymType::INT_TYPE;
            }
        }
        else if(firstType == SymType::CHAR_TYPE) {
            if(secType == SymType::DOUBLE_TYPE) {
                _instructions[funcIndex].emplace_back(Operation::D2I);
                _instructions[funcIndex].emplace_back(Operation::I2C);
                type = SymType::CHAR_TYPE;
            }
            else if(secType == SymType::INT_TYPE) {
                _instructions[funcIndex].emplace_back(Operation::I2C);
                type = SymType::CHAR_TYPE;
            }
            else if(secType == SymType::CHAR_TYPE)
                type = SymType::CHAR_TYPE;
        }
        else if(firstType == SymType::DOUBLE_TYPE) {
            if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                // 把表达式的值转换为 double
                _instructions[funcIndex].emplace_back(Operation::I2D);
            type = SymType::DOUBLE_TYPE;
        }

        // 生成指令，将栈顶的值存入上述地址
        if(type == SymType::DOUBLE_TYPE)
            _instructions[funcIndex].emplace_back(Operation::DSTORE);
        else
            _instructions[funcIndex].emplace_back(Operation::ISTORE);

        return {};
    }

    // <condition> ::= <expression>[<relational-operator><expression>]
    //                   次栈顶 lhs                        栈顶 rhs
    // <condition> 判断完后
    // 如果 lhs == rhs，栈顶为 0
    // 如果 lhs > rhs，栈顶为 1
    // 如果 lhs < rhs，栈顶为 -1
    std::optional<CompilationError> Analyser::analyseCondition(TokenType& type, int32_t funcIndex) {
        // <expression>
        SymType firstType;
        auto err = analyseExpression(firstType, funcIndex);
        if(err.has_value())
            return err;

        // [<relational-operator><expression>]
        // <relational-operator> ::= '<' | '<=' | '>' | '>=' | '!=' | '=='
        auto opt = nextToken();
        if(!opt.has_value())
            return {};
        if(opt.value().GetType() != TokenType::LESS_SIGN &&
           opt.value().GetType() != TokenType::LESS_EQUAL_SIGN &&
           opt.value().GetType() != TokenType::GREATER_SIGN &&
           opt.value().GetType() != TokenType::GREATER_EQUAL_SIGN &&
           opt.value().GetType() != TokenType::NONEQUAL_SIGN &&
           opt.value().GetType() != TokenType::EQUAL_SIGN) {
            unreadToken();
            // 说明这里是 <condition> ::= <expression>
            // 如果<expression>是（或可以转换为）int类型，且转换得到的值为0，那么视为false；否则均视为true。
            if(firstType == DOUBLE_TYPE)
                _instructions[funcIndex].emplace_back(Operation::D2I);
            return {};
        }
        type = opt.value().GetType();

        auto iter = _instructions[funcIndex].end();

        // <expression>
        SymType secType;
        err = analyseExpression(secType, funcIndex);
        if(err.has_value())
            return err;

        // 生成隐式类型转换指令
        // 设置表达式数据类型
        SymType conditionType;
        if(firstType == SymType::INT_TYPE || firstType == SymType::CHAR_TYPE) {
            if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                conditionType = SymType::INT_TYPE;
            else if(secType == SymType::DOUBLE_TYPE) {
                // 把第一个表达式的值转换为 double
                _instructions[funcIndex].insert(iter, Operation::I2D);
                conditionType = SymType::DOUBLE_TYPE;
            }
        } else if(firstType == SymType::DOUBLE_TYPE) {
            if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                // 把第二个表达式的值转换为 double
                _instructions[funcIndex].emplace_back(Operation::I2D);
            conditionType = SymType::DOUBLE_TYPE;
        }

        // 添加指令
        // 将两个结果进行比较
        if(conditionType == SymType::DOUBLE_TYPE)
            _instructions[funcIndex].emplace_back(Operation::DCMP);
        else
            _instructions[funcIndex].emplace_back(Operation::ICMP);

        return {};
    }


    // <expression> ::= <additive-expression>
    // <additive-expression> ::= <multiplicative-expression>{<additive-operator><multiplicative-expression>}
    std::optional<CompilationError> Analyser::analyseExpression(SymType& type, int32_t funcIndex) {
        // <multiplicative-expression>
        SymType firstType;
        auto err = analyseMultiplicativeExpression(firstType, funcIndex);
        if(err.has_value())
            return err;

        type = firstType;

        // {<additive-operator><multiplicative-expression>}
        // <additive-operator> ::= '+' | '-'
        while(true) {
            auto opt = nextToken();
            if(!opt.has_value())
                return {};
            if(opt.value().GetType() != TokenType::PLUS_SIGN && opt.value().GetType() != TokenType::MINUS_SIGN) {
                unreadToken();
                return {};
            }

            // 记录表达式左边的位置，有可能需要插入类型转换指令
            auto iter = _instructions[funcIndex].end();

            // <multiplicative-expression>
            SymType secType;
            err = analyseMultiplicativeExpression(secType, funcIndex);
            if(err.has_value())
                return err;

            // std::cout << "tmp tmp tmp type = " << secType << std::endl;

            // 生成隐式类型转换指令
            // 设置表达式数据类型
            if(firstType == SymType::INT_TYPE || firstType == SymType::CHAR_TYPE) {
                if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                    type = SymType::INT_TYPE;
                else if(secType == SymType::DOUBLE_TYPE) {
                    // 把第一个表达式的值转换为 double
                    _instructions[funcIndex].insert(iter, Operation::I2D);
                    type = SymType::DOUBLE_TYPE;
                }
            } else if(firstType == SymType::DOUBLE_TYPE) {
                if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                    // 把第二个表达式的值转换为 double
                    _instructions[funcIndex].emplace_back(Operation::I2D);
                type = SymType::DOUBLE_TYPE;
            }

            // std::cout << "add expression type = " << type << std::endl;

            // 加减指令压栈
            if(opt.value().GetType() == TokenType::PLUS_SIGN)
                if(type == SymType::DOUBLE_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::DADD);
                else
                    _instructions[funcIndex].emplace_back(Operation::IADD);
            else
                if(type == SymType::DOUBLE_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::DSUB);
                else
                    _instructions[funcIndex].emplace_back(Operation::ISUB);
        }
    }

    // <multiplicative-expression> ::= <unary-expression>{<multiplicative-operator><unary-expression>}
    // 下面是增加了类型转换的
    // <multiplicative-expression> ::= <cast-expression>{<multiplicative-operator><cast-expression>}
    // 如果两操作数类型不同，应隐式地将较小的类型转换至较大的类型，最终得到的结果类型和类型较大的操作数一致
    std::optional<CompilationError> Analyser::analyseMultiplicativeExpression(SymType& type, int32_t funcIndex) {
	    // <cast-expression>
	    SymType firstType;
	    auto err = analyseCastExpression(firstType, funcIndex);
	    if(err.has_value())
            return err;

	    // 先为 type 赋值，后面如果有 opt 会再更改
	    type = firstType;

	    // {<multiplicative-operator><unary-expression>}
	    while(true) {
	        auto opt = nextToken();
	        if(!opt.has_value())
                return {};
	        if(opt.value().GetType() != TokenType::MULTIPLICATION_SIGN && opt.value().GetType() != TokenType::DIVISION_SIGN) {
	            unreadToken();
                return {};
	        }

	        // 记录表达式左边的位置，有可能需要插入类型转换指令
	        auto iter = _instructions[funcIndex].end();

            // <cast-expression>
            SymType secType;
            err = analyseCastExpression(secType, funcIndex);
            if(err.has_value())
                return err;

            // 生成隐式类型转换指令
            // 设置表达式数据类型
            if(firstType == SymType::INT_TYPE) {
                if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                    type = SymType::INT_TYPE;
                else if(secType == SymType::DOUBLE_TYPE) {
                    // 把第一个表达式的值转换为 double
                    _instructions[funcIndex].insert(iter, Operation::I2D);
                    type = SymType::DOUBLE_TYPE;
                }
            } else if(firstType == SymType::DOUBLE_TYPE) {
                if(secType == SymType::INT_TYPE || secType == SymType::CHAR_TYPE)
                    // 把第二个表达式的值转换为 double
                    _instructions[funcIndex].emplace_back(Operation::I2D);
                type = SymType::DOUBLE_TYPE;
            }

            // std::cout << "Multi expression type = " << type << std::endl;

            // 将乘/除指令压栈
            if(opt.value().GetType() == TokenType::DIVISION_SIGN)
                if(type == SymType::DOUBLE_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::DDIV);
                else
                    _instructions[funcIndex].emplace_back(Operation::IDIV);
            else
                if(type == SymType::DOUBLE_TYPE)
                    _instructions[funcIndex].emplace_back(Operation::DMUL);
                else
                    _instructions[funcIndex].emplace_back(Operation::IMUL);
	    }
	}

	// <cast-expression> ::= {'('<type-specifier>')'}<unary-expression>
    std::optional<CompilationError> Analyser::analyseCastExpression(SymType& type, int32_t funcIndex) {
	    // {'('<type-specifier>')'}
	    // 需要预读两个
	    std::vector<SymType> types;
	    while(true) {
	        // '('
            auto next = nextToken();
            if(!next.has_value())
                break;
            if(next.value().GetType() != TokenType::LEFT_BRACKET) {
                unreadToken();
                break;
            }

            // <type-specifier>
            auto specifier = nextToken();
            if(!specifier.has_value())
                break;
            if(specifier.value().GetType() != TokenType::VOID &&
                specifier.value().GetType() != TokenType::INT  &&
                specifier.value().GetType() != TokenType::CHAR &&
                specifier.value().GetType() != TokenType::DOUBLE) {
                unreadToken(); // 回退这个
                unreadToken(); // 回退上面的 （
                break;
            }

            switch(specifier.value().GetType()) {
                case VOID:
                    // 只要<cast-expression>的目标类型是void，无论操作数<unary-expression>的类型是什么，都是语义错误
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                case INT:
                    types.emplace_back(SymType::INT_TYPE);
                    break;
                case CHAR:
                    types.emplace_back(SymType::CHAR_TYPE);
                    break;
                case DOUBLE:
                    types.emplace_back(SymType::DOUBLE_TYPE);
                default:
                    break;
            }

            // ')'
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidCastExpression);

	    }

        // <unary-expression>
        SymType unaryType;
	    auto err = analyseUnaryExpression(unaryType, funcIndex);
	    if(err.has_value())
            return err;

	    // 无论<cast-expression>的目标类型是什么，只要操作数<unary-expression>的类型是void，都是语义错误
	    if(unaryType == SymType::VOID_TYPE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

        type = unaryType;

        // 转换操作
        while(!types.empty()) {
            SymType tmp = types.back();

            // std::cout << "change type = " << tmp << std::endl;

            types.pop_back();
            switch(tmp) {
                case INT_TYPE: {
                    if(type == SymType::INT_TYPE || type == SymType::CHAR_TYPE)
                        ;
                    else if(type == SymType::DOUBLE_TYPE)
                        _instructions[funcIndex].emplace_back(Operation::D2I); // 把 double 转换为 int
                    else {}
                    // 设置表达式类型为 int
                    type = SymType::INT_TYPE;
                    break;
                }
                case DOUBLE_TYPE: {
                    if(type == SymType::INT_TYPE || type == SymType::CHAR_TYPE)
                        _instructions[funcIndex].emplace_back(Operation::I2D);
                    else if(type == SymType::DOUBLE_TYPE)
                        ;
                    else
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                    type = SymType::DOUBLE_TYPE;
                    break;
                }
                case CHAR_TYPE: {
                    if(type == SymType::INT_TYPE)
                        _instructions[funcIndex].emplace_back(Operation::I2C);
                    else if(type == SymType::DOUBLE_TYPE) {
                        _instructions[funcIndex].emplace_back(Operation::D2I);
                        _instructions[funcIndex].emplace_back(Operation::I2C);
                    }
                    else if(type == SymType::CHAR_TYPE)
                        ;
                    else
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                    type = SymType::CHAR_TYPE;
                    break;
                }
                default:
                    break;
            }
        }

        // std::cout << "Cast expression type = " << type << std::endl;

        return {};
	}

    // <unary-expression> ::= [<unary-operator>]<primary-expression>
    // <unary-operator> ::= '+' | '-'
    std::optional<CompilationError> Analyser::analyseUnaryExpression(SymType& type, int32_t funcIndex) {
	    // <unary-operator>
        auto opt = nextToken();
        if(!opt.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidUnaryExpression);
        int flag = true;
        if(opt.value().GetType() == TokenType::PLUS_SIGN)
            ;
        else if(opt.value().GetType() == TokenType::MINUS_SIGN)
            flag = false;
        else
            unreadToken();

        auto err = analysePrimaryExpression(type, funcIndex);
        if(err.has_value())
            return err;

        // std::cout << "Unary expression type = " << type << std::endl;

        // 添加取负指令
        if(!flag) {// 说明是取负
            Operation tmp;
            if(type == SymType::INT_TYPE)
                tmp = Operation::INEG;
            else if(type == SymType::DOUBLE_TYPE)
                tmp = Operation::DNEG;
            _instructions[funcIndex].emplace_back(tmp);
        }

        return {};
	}

    // <primary-expression> ::= '('<expression>')' | <identifier> | <integer-literal> | <function-call>
    // <function-call> ::= <identifier> '(' [<expression-list>] ')'
    // <primary-expression> 的类型和值，与其推导出的语法成分完全相同
    std::optional<CompilationError> Analyser::analysePrimaryExpression(SymType& type, int32_t funcIndex) {
	    auto next = nextToken();
	    if(!next.has_value() ||
	      (next.value().GetType() != TokenType::LEFT_BRACKET &&
	       next.value().GetType() != TokenType::INTEGER &&
	       next.value().GetType() != TokenType::CHAR_TOKEN &&
	       next.value().GetType() != TokenType::IDENTIFIER)
	       )
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrimaryExpression);

	    switch(next.value().GetType()) {
	        case LEFT_BRACKET: { // '('<expression>')'
	            auto err = analyseExpression(type, funcIndex);
	            if(err.has_value())
                    return err;
	            auto next = nextToken();
	            if(!next.has_value() || next.value().GetType() != RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrimaryExpression);
	            break;
            }
            case INTEGER: { // <integer-literal>
                // 读到数字直接压栈了
                int32_t val;
                try {
                    val = std::any_cast<int32_t>(next.value().GetValue());
                } catch(const std::bad_any_cast&) {
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIntegerOverflow);
                }
                // 设置此处类型为 int
                type = SymType::INT_TYPE;
                // 如果是大字节数据就添加到常量表，然后添加 loadc 指令
                // 将数字压栈
                _instructions[funcIndex].emplace_back(Operation::IPUSH, val);
                break;
            }
            case CHAR_TOKEN: {
                char ch;
                try {
                    std::string str = std::any_cast<std::string>(next.value().GetValue());
                    ch = *(str.c_str());
                    // std::cout << "ch=" << ch <<std::endl;
                } catch(const std::bad_any_cast&) {
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCharInvalid);
                }
                type = SymType::INT_TYPE;
                _instructions[funcIndex].emplace_back(Operation::BIPUSH, ch);
                break;
            }
            case IDENTIFIER: {
                auto pre_next = nextToken();
                unreadToken();
                if(pre_next.has_value() && pre_next.value().GetType() == TokenType::LEFT_BRACKET) {
                    // 说明这里是 <function-call>
                    unreadToken(); // 再把 identifier 回溯
                    auto err = analyseFunctionCall(type, funcIndex);
                    // 表达式里不能用 void
                    if(type == SymType::VOID_TYPE)
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrimaryExpression);
                    if(err.has_value())
                        return err;
                } else {
                    // 说明这里是 <identifier>
                    // 查符号表，必须存在、已初始化
                    // 是全局变量还是局部变量
                    bool isGlobal = false;
                    // 查符号表: 已声明、局部符号表必然没有func，全局变量需要判断一下不能是函数
                    // int 还是 double
                    if(!isDeclared(funcIndex, next.value().GetValueString())) {
                        if(!isDeclared(-1, next.value().GetValueString()))
                            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
                        // 说明是全局变量
                        isGlobal = true;
                        if(!isInit(-1, next.value().GetValueString()))
                            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
                        type = getVarType(-1, next.value().GetValueString());
                    } else {
                        // 说明是局部变量
                        if(!isInit(funcIndex, next.value().GetValueString()))
                            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
                        type = getVarType(funcIndex, next.value().GetValueString());
                    }

                    // std::cout << "Primary expression type = " << type << std::endl;

                    // 生成指令
                    int16_t level_diff;
                    int32_t offset;
                    if(isGlobal) {
                        offset = getVarIndex(-1, next.value().GetValueString());
                        if(funcIndex == -1) // 说明这里是全局变量的初始化
                            level_diff = 0;
                        else
                            level_diff = 1; // 说明这里是函数体内调用全局变量
                    } else { // 这里是局部变量，那么只能是局部变量在函数体内被调用了
                        offset = getVarIndex(funcIndex, next.value().GetValueString());
                        level_diff = 0;
                    }

                    // 加载变量的地址
                    _instructions[funcIndex].emplace_back(Operation::LOADA, level_diff, offset);
                    // 从栈中弹出地址，从地址处加载数据压栈
                    if(type == SymType::DOUBLE_TYPE)
                        _instructions[funcIndex].emplace_back(Operation::DLOAD);
                    else
                        _instructions[funcIndex].emplace_back(Operation::ILOAD);
                }
                break;
            }
            default:
                break;
	    }

        return {};

	}

	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}

    void Analyser::addConstant(std::string name, SymType type) {
	    _constant_symbols.addVar(name, true, type);
	}

	void Analyser::addVar(int32_t funcIndex, std::string name, bool isConst, cc0::SymType type) {
	    if(_var_symbols.find(funcIndex) == _var_symbols.end()) // 函数刚创建
            _var_symbols[funcIndex] = *new SymTable;
	    _var_symbols[funcIndex].addVar(name, isConst, type);
	}

    int32_t Analyser::addFunc(std::string name, SymType type) {
        return _constant_symbols.addFunc(name, type);
	}

    bool Analyser::isMainExisted() {
        return _constant_symbols.isMainExisted();
    }

    bool Analyser::isDeclaredFunc(std::string name) {
        return _constant_symbols.isFunction(name);
	}

    bool Analyser::isDeclared(int32_t funcIndex, std::string name) {
	    // 如果是全局变量需要同时查全局变量表和函数表
	    if(funcIndex == -1)
            return _constant_symbols.isFunction(name) || _var_symbols[-1].isDeclared(name);
        return _var_symbols[funcIndex].isDeclared(name);
	}

    bool Analyser::isConstantExisted(SymType type, std::string name) {
        return _constant_symbols.isConstantExisted(type, name);
	}

    bool Analyser::isInit(int32_t funcIndex, std::string name) {
            return _var_symbols[funcIndex].isInit(name);
	}

	int32_t Analyser::getFuncParamNum(std::string name) {
        return _constant_symbols.getFuncParamNum(name);
	}

    void Analyser::setFuncParamNum(std::string name, int32_t param_num) {
        _constant_symbols.setFuncParamNum(name, param_num);
	}

	SymType Analyser::getFuncType(std::string name) {
        return _constant_symbols.getFuncType(name);
	}

	SymType Analyser::getFuncParamType(int32_t funcIndex, int32_t paramIndex) {
        return _var_symbols[funcIndex].getFuncParamType(paramIndex);
	}

	int32_t Analyser::getConstantIndex(std::string name) {
        return _constant_symbols.getIndex(name);
	}

	int32_t Analyser::getFuncOrder(std::string name) {
        return _constant_symbols.getFuncOrder(name);
	}

    std::string Analyser::getFuncName(int32_t funcIndex) {
        return _constant_symbols.getNameByIndex(funcIndex);
    }

	int32_t Analyser::getVarIndex(int32_t funcIndex, std::string name) {
        return _var_symbols[funcIndex].getVarIndex(name);
	}

	SymType Analyser::getVarType(int32_t funcIndex, std::string name) {
        return _var_symbols[funcIndex].getType(name);
	}

	bool Analyser::isConst(int32_t funcIndex, std::string name) {
	    return _var_symbols[funcIndex].isConst(name);
	}

	void Analyser::initVar(int32_t funcIndex, std::string name) {
	        _var_symbols[funcIndex].initVar(name);
	}

	void Analyser::printSym() {
	    std::cout << "constant: " << std::endl;
	    _constant_symbols.print();
        std::cout << "var: " << std::endl;
        auto iter = _var_symbols.begin();
        for(; iter!=_var_symbols.end(); iter++) {
            std::cout << "funcIndex: " << iter->first << std::endl;
            iter->second.print();
        }
	}

}