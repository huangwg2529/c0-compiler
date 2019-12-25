#pragma once

#include "error/error.h"
#include "instruction/instruction.h"
#include "tokenizer/token.h"
#include "symTable.h"

#include <vector>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstddef> // for std::size_t

namespace cc0 {

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
		using int16_t = std::int16_t;
	public:
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _current_pos(0, 0) {}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;

		// 对外接口：返回生成的指令集或报错
		std::pair<std::map<int32_t ,std::vector<Instruction>>, std::optional<CompilationError>> Analyse();
        // 提供全局符号表，在生成汇编体和常量表时需要使用
        std::vector<Symbol> getConstants() { return _constant_symbols.getSymbols(); };
        // 获取函数数量
        int32_t getFuncSize() { return _constant_symbols.getFuncSize(); };
		void printSym();

	private:
		// 所有的递归子程序

		// <C0-program>
		std::optional<CompilationError> analyseC0Program();
		// <variable-declaration>
		std::optional<CompilationError> analyseVariableDeclaration(int32_t funcIndex);
        // <init-declarator-list>
        std::optional<CompilationError> analyseInitDeclaratorList(int32_t funcIndex, bool isConst, SymType& type);
        // <init-declarator>
        std::optional<CompilationError> analyseInitDeclarator(int32_t funcIndex, bool isConst, SymType& type);

        // <function-definition>
        std::optional<CompilationError> analyseFunctionDefinition();
        // <parameter-declaration-list>
        std::optional<CompilationError> analyseParameterDeclarationList(int32_t funcIndex, int32_t& param_num);
        // <parameter-declaration>
        std::optional<CompilationError> analyseParameterDeclaration(int32_t funcIndex);
        // <function-call>
        std::optional<CompilationError> analyseFunctionCall(SymType& type, int32_t funcIndex);
        // <expression-list>
        // 第一个参数是指当前在哪个函数体内，第二个参数是指被调用的函数是哪个
        std::optional<CompilationError> analyseExpressionList(int32_t funcIndex, int32_t constIndex, int32_t param_num);

        // <compound-statement>
        std::optional<CompilationError> analyseCompoundStatement(int32_t funcIndex);
        // <statement-seq>
        std::optional<CompilationError> analyseStatementSeq(int32_t funcIndex, bool& isReturn);
        // <statement>
        std::optional<CompilationError> analyseStatement(int32_t funcIndex, bool& isReturn);
        // <condition-statement>
        std::optional<CompilationError> analyseConditionStatement(int32_t funcIndex, bool& isReturn);
        // <loop-statement>
        std::optional<CompilationError> analyseLoopStatement(int32_t funcIndex, bool& isReturn);
        // <jump-statement>
        std::optional<CompilationError> analyseJumpStatement(int32_t funcIndex);
        // <print-statement>
        std::optional<CompilationError> analysePrintStatement(int32_t funcIndex);
        // <printable-list>
        std::optional<CompilationError> analysePrintableList(int32_t funcIndex);
        // <printable>
        std::optional<CompilationError> analysePrintable(int32_t funcIndex);
        // <scan-statement>
        std::optional<CompilationError> analyseScanStatement(int32_t funcIndex);



        // <assignment-expression>
        std::optional<CompilationError> analyseAssignmentExpression(int32_t funcIndex);
        // <condition>
        std::optional<CompilationError> analyseCondition(TokenType& type, int32_t funcIndex);
        // <expression>
        std::optional<CompilationError> analyseExpression(SymType& type, int32_t funcIndex);
        // <multiplicative-expression>
        std::optional<CompilationError> analyseMultiplicativeExpression(SymType& type, int32_t funcIndex);
        // <cast-expression>
        std::optional<CompilationError> analyseCastExpression(SymType& type, int32_t funcIndex);
        // <unary-expression>
        std::optional<CompilationError> analyseUnaryExpression(SymType& type, int32_t funcIndex);
        // <primary-expression>
        std::optional<CompilationError> analysePrimaryExpression(SymType& type, int32_t funcIndex);


		// Token 缓冲区相关操作

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();

		// 下面是符号表相关操作
		// 添加
		void addVar(int32_t funcIndex, std::string name, bool isConst, SymType type);
		// 添加函数，并返回函数位置
		int32_t addFunc(std::string name, SymType type);
        void addConstant(std::string name, SymType type);
		// 是否已声明
        bool isMainExisted();
        bool isDeclaredFunc(std::string name);
		bool isDeclared(int32_t funcIndex, std::string name);
        // 字符串字面量是否已存在
        bool isConstantExisted(SymType type, std::string name);
		// 变量是否初始化
		bool isInit(int32_t funcIndex, std::string name);

		// 获取参数数量，如果不是函数，返回-1
        int32_t getFuncParamNum(std::string name);
        void setFuncParamNum(std::string name, int32_t param_num);
        // 获取函数返回值类型
        SymType getFuncType(std::string name);
        // 获取函数参数的类型
        SymType getFuncParamType(int32_t funcIndex, int32_t paramIndex);
        // 获取函数名
        std::string getFuncName(int32_t funcIndex);

        // 获取标识符在常量表中的位置
        int32_t getConstantIndex(std::string name);
        // 无视其他常量，只看函数是第几个
        int32_t getFuncOrder(std::string name);
        // 获取变量在符号表中的索引
        int32_t getVarIndex(int32_t funcIndex, std::string name);
        // 获取变量类型
        bool isConst(int32_t funcIndex, std::string name);
        SymType getVarType(int32_t funcIndex, std::string name);
        // 设置变量为已初始化
        void initVar(int32_t funcIndex, std::string name);

	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
		std::pair<uint64_t, uint64_t> _current_pos;

        // 常量表：存储函数符号、某些大字节的东西比如字符串字面量
        SymTable _constant_symbols;
        // 符号表，key 是函数在常量表的位置
        // key == -1 时，存储全局变量
        // 否则是函数里的局部变量， key 对应常量表的函数符号
        std::map<int32_t, SymTable> _var_symbols;

        // 生成指令集
        // key 是函数在常量表的位置
        // key = -1 表示启动代码，否则就是对应函数体的内容
        std::map<int32_t, std::vector<Instruction>> _instructions;
	};
}
