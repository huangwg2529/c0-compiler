#include "fmt/core.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"

namespace fmt {
	template<>
	struct formatter<cc0::ErrorCode> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::ErrorCode &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case cc0::ErrNoError:
				name = "No error.";
				break;
			case cc0::ErrStreamError:
				name = "Stream error.";
				break;
			case cc0::ErrEOF:
				name = "EOF";
				break;
			case cc0::ErrInvalidInput:
				name = "The input is invalid.";
				break;
            case cc0::ErrInvalidInteger:
                name = "The integer is invalid.";
                break;
			case cc0::ErrInvalidIdentifier:
				name = "Identifier is invalid";
				break;
			case cc0::ErrIntegerOverflow:
				name = "The integer is too big(int64_t).";
				break;
            case cc0::ErrCharInvalid:
                name = "Char is invalid.";
                break;
            case cc0::ErrStringInvalid:
                name = "String is invalid.";
                break;
            case cc0::ErrNeedMain:
                name = "Need a main function.";
                break;
            case cc0::ErrNeedType:
                name = "Need a type specifier here.";
                break;
            case cc0::ErrInvalidFunctionDefinition:
                name = "The function Definition is invalid.";
                break;
            case cc0::ErrInvalidFunctionCall:
                name = "The function call is invalid.";
                break;
            case cc0::ErrCallUndefined:
                name = "The function you called is undefined.";
                break;
			    case cc0::ErrParamsInvalid:
			        name = "The parameters of the function is incorrect.";
			        break;
            case cc0::ErrVariableVoid:
                name = "The type void cannot be used to variable declaration.";
                break;
            case cc0::ErrInvalidCompoundStatement:
                name = "The compound statement is invalid.";
                break;
            case cc0::ErrInvalidStatementSeq:
                name = "The statement sequence is invalid.";
                break;
            case cc0::ErrInvalidConditionStatement:
                name = "The condition statement is invalid.";
                break;
            case cc0::ErrInvalidLoopStatement:
                name = "The loop statement is invalid.";
                break;
            case cc0::ErrInvalidReturnStatement:
                name = "The return statement is invalid.";
                break;
            case cc0::ErrInvalidPrintStatement:
                name = "The print statement is invalid.";
                break;
            case cc0::ErrInvalidScanStatement:
                name = "The scan statement is invalid.";
                break;
            case cc0::ErrInvalidCastExpression:
                name = "The cast expression is invalid.";
                break;
            case cc0::ErrInvalidUnaryExpression:
                name = "The unary expression is invalid.";
                break;
            case cc0::ErrInvalidType:
                name = "The type of the expression is invalid somewhere.";
                break;
            case cc0::ErrInvalidPrimaryExpression:
                name = "The primary expression is invalid.";
                break;
            case cc0::ErrExpressionType:
                name = "The type of the expression is error.";
                break;


			case cc0::ErrNoBegin:
				name = "The program should start with 'begin'.";
				break;
			case cc0::ErrNoEnd:
				name = "The program should end with 'end'.";
				break;
			case cc0::ErrNeedIdentifier:
				name = "Need an identifier here.";
				break;
			case cc0::ErrConstantNeedValue:
				name = "The constant need a value to initialize.";
				break;
			case cc0::ErrNoSemicolon:
				name = "Zai? Wei shen me bu xie fen hao.";
				break;
			case cc0::ErrInvalidVariableDeclaration:
				name = "The declaration is invalid.";
				break;
			case cc0::ErrIncompleteExpression:
				name = "The expression is incomplete.";
				break;
			case cc0::ErrNotDeclared:
				name = "The variable or constant must be declared before being used.";
				break;
			case cc0::ErrAssignToConstant:
				name = "Trying to assign value to a constant.";
				break;
			case cc0::ErrDuplicateDeclaration:
				name = "The variable or constant or function has been declared.";
				break;
			case cc0::ErrNotInitialized:
				name = "The variable has not been initialized.";
				break;
			case cc0::ErrInvalidAssignment:
				name = "The assignment statement is invalid.";
				break;
			case cc0::ErrInvalidPrint:
				name = "The output statement is invalid.";
				break;
            case cc0::ErrHexademicalChange:
                name = "Error happened when trying to change string to hex integer.";
                break;
            case cc0::ErrIncompleteCommit:
                name = "The commit is incomplete.";
                break;
            case cc0::ErrTest:
                name = "Test.";
                break;
			}
			return format_to(ctx.out(), name);
		}
	};

	template<>
	struct formatter<cc0::CompilationError> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::CompilationError &p, FormatContext &ctx) {
			return format_to(ctx.out(), "Line: {} Column: {} Error: {}", p.GetPos().first, p.GetPos().second, p.GetCode());
		}
	};
}

namespace fmt {
	template<>
	struct formatter<cc0::Token> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::Token &p, FormatContext &ctx) {
			return format_to(ctx.out(),
				"Line: {} Column: {} Type: {} Value: {}",
				p.GetStartPos().first, p.GetStartPos().second, p.GetType(), p.GetValueString());
		}
	};

	template<>
	struct formatter<cc0::TokenType> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::TokenType &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case cc0::NULL_TOKEN:
				name = "NullToken";
				break;
			case cc0::INTEGER:
				name = "Integer";
				break;
			case cc0::IDENTIFIER:
				name = "Identifier";
				break;
            case cc0::CHAR_TOKEN:
                name = "char";
                break;
            case cc0::STRING:
                name = "string";
                break;
			case cc0::CONST:
				name = "Const";
				break;
            case cc0::VOID:
                name = "Void";
                break;
            case cc0::INT:
                name = "Int";
                break;
            case cc0::CHAR:
                name = "Char";
                break;
            case cc0::DOUBLE:
                name = "Double";
                break;
            case cc0::STRUCT:
                name = "Struct";
                break;
			case cc0::PRINT:
				name = "Print";
				break;
            case cc0::IF:
                name = "If";
                break;
            case cc0::ELSE:
                name = "Name";
                break;
            case cc0::SWITCH:
                name = "Switch";
                break;
            case cc0::CASE:
                name = "Case";
                break;
            case cc0::DEFAULT:
                name = "Default";
                break;
            case cc0::WHILE:
                name = "While";
                break;
            case cc0::FOR:
                name = "For";
                break;
            case cc0::DO:
                name = "Do";
                break;
            case cc0::RETURN:
                name = "Return";
                break;
            case cc0::CONTINUE:
                name = "Continue";
                break;
            case cc0::SCAN:
                name = "Scan";
                break;
			case cc0::PLUS_SIGN:
				name = "PlusSign";
				break;
			case cc0::MINUS_SIGN:
				name = "MinusSign";
				break;
			case cc0::MULTIPLICATION_SIGN:
				name = "MultiplicationSign";
				break;
			case cc0::DIVISION_SIGN:
				name = "DivisionSign";
				break;
			case cc0::EQUAL_SIGN:
				name = "EqualSign";
				break;
			case cc0::SEMICOLON:
				name = "Semicolon";
				break;
			case cc0::LEFT_BRACKET:
				name = "LeftBracket";
				break;
			case cc0::RIGHT_BRACKET:
				name = "RightBracket";
				break;
            case cc0::LEFT_BRACE:
                name = "LeftBrace";
                break;
            case cc0::RIGHT_BRACE:
                name = "RightBrace";
                break;
            case cc0::ASSIGN_SIGN:
                name = "AssignSign";
                break;
            case cc0::LESS_SIGN:
                name = "LessSign";
                break;
            case cc0::LESS_EQUAL_SIGN:
                name = "LessEqualSign";
                break;
            case cc0::GREATER_SIGN:
                name = "GreaterSign";
                break;
            case cc0::GREATER_EQUAL_SIGN:
                name = "GreaterEqualSign";
                break;
            case cc0::NONEQUAL_SIGN:
                name = "NonEqualSign";
                break;
            case cc0::COMMA_SIGN:
                name = "CommaSign";
                break;
            default:
                break;
			}
			return format_to(ctx.out(), name);
		}
	};
}

namespace fmt {
	template<>
	struct formatter<cc0::Operation> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::Operation &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case cc0::NOP:
				name = "nop";
				break;
			case cc0::BIPUSH:
				name = "bipush";
				break;
			case cc0::IPUSH:
				name = "ipush";
				break;
			case cc0::POP:
				name = "pop";
				break;
			case cc0::POP2:
				name = "pop2";
				break;
			case cc0::POPN:
				name = "popn";
				break;
			case cc0::DUP:
				name = "dup";
				break;
			case cc0::DUP2:
				name = "dup2";
				break;
			case cc0::LOADC:
				name = "loadc";
				break;
            case cc0::LOADA:
                name = "loada";
            break;
            case cc0::NEW:
                name = "new";
            break;
            case cc0::SNEW:
                name = "snew";
            break;
            case cc0::ILOAD:
                name = "iload";
            break;
            case cc0::DLOAD:
                name = "dload";
            break;
            case cc0::ALOAD:
                name = "aload";
            break;
            case cc0::IALOAD:
                name = "iaload";
            break;
            case cc0::DALOAD:
                name = "daload";
            break;
            case cc0::AALOAD:
                name = "aaload";
                break;
            case cc0::ISTORE:
                name = "istore";
                break;
            case cc0::DSTORE:
                name = "dstore";
                break;
            case cc0::ASTORE:
                name = "astore";
                break;
            case cc0::IASTORE:
                name = "iastore";
                break;
            case cc0::DASTORE:
                name = "dastore";
                break;
            case cc0::AASTORE:
                name = "aastore";
                break;
            case cc0::IADD:
                name = "iadd";
                break;
            case cc0::DADD:
                name = "dadd";
                break;
            case cc0::ISUB:
                name = "isub";
                break;
            case cc0::DSUB:
                name = "dsub";
                break;
            case cc0::IMUL:
                name = "imul";
                break;
            case cc0::DMUL:
                name = "dmul";
                break;
            case cc0::IDIV:
                name = "idiv";
                break;
            case cc0::DDIV:
                name = "ddiv";
                break;
            case cc0::INEG:
                name = "ineg";
                break;
            case cc0::DNEG:
                name = "dneg";
                break;
            case cc0::ICMP:
                name = "icmp";
                break;
            case cc0::DCMP:
                name = "dcmp";
                break;
            case cc0::I2D:
                name = "i2d";
                break;
            case cc0::D2I:
                name = "d2i";
                break;
            case cc0::I2C:
                name = "i2c";
                break;
            case cc0::JMP:
                name = "jmp";
                break;
            case cc0::JE:
                name = "je";
                break;
            case cc0::JNE:
                name = "jne";
                break;
            case cc0::JL:
                name = "jl";
                break;
            case cc0::JGE:
                name = "jge";
                break;
            case cc0::JG:
                name = "jg";
                break;
            case cc0::JLE:
                name = "jle";
                break;
            case cc0::CALL:
                name = "call";
                break;
            case cc0::RET:
                name = "ret";
                break;
            case cc0::IRET:
                name = "iret";
                break;
            case cc0::DRET:
                name = "dret";
                break;
            case cc0::ARET:
                name = "aret";
                break;
            case cc0::IPRINT:
                name = "iprint";
                break;
            case cc0::DPRINT:
                name = "dprint";
                break;
            case cc0::CPRINT:
                name = "cprint";
                break;
            case cc0::SPRINT:
                name = "sprint";
                break;
            case cc0::PRINTL:
                name = "printl";
                break;
            case cc0::ISCAN:
                name = "iscan";
                break;
            case cc0::DSCAN:
                name = "dscan";
                break;
            case cc0::CSCAN:
                name = "cscan";
                break;
        }
			return format_to(ctx.out(), name);
		}
	};
	template<>
	struct formatter<cc0::Instruction> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const cc0::Instruction &p, FormatContext &ctx) {
		    if(p.getX() == -1 && p.getY() == -1)
                return format_to(ctx.out(), "{}", p.getOperation());
		    if(p.getX() != -1 && p.getY() == -1)
                return format_to(ctx.out(), "{} {}", p.getOperation(), p.getX());
            if(p.getX() == -1 && p.getY() != -1)
                return format_to(ctx.out(), "{} {}", p.getOperation(), p.getY());
            return format_to(ctx.out(), "{} {}, {}", p.getOperation(), p.getX(), p.getY());
		}
	};
}