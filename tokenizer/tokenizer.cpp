#include "tokenizer/tokenizer.h"

#include <cctype>
#include <sstream>

namespace cc0 {

    const std::map<std::string, TokenType> reservedKeys = {
            {"const", TokenType::CONST},
            {"void", TokenType::VOID},
            {"int", TokenType::INT},
            {"char", TokenType::CHAR},
            {"double", TokenType::DOUBLE},
            {"struct", TokenType::STRUCT},
            {"if", TokenType::IF},
            {"else", TokenType::ELSE},
            {"switch", TokenType::SWITCH},
            {"case", TokenType::CASE},
            {"default", TokenType::DEFAULT},
            {"while", TokenType::WHILE},
            {"for", TokenType::FOR},
            {"do", TokenType::DO},
            {"return", TokenType::RETURN},
            {"break", TokenType::RETURN},
            {"continue", TokenType::CONTINUE},
            {"print", TokenType::PRINT},
            {"scan", TokenType::SCAN}
    };

	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
		if (!_initialized)
			readAll();
		if (_rdr.bad())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
		if (isEOF())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
		auto p = nextToken();
		if (p.second.has_value())
			return std::make_pair(p.first, p.second);
		auto err = checkToken(p.first.value());
		if (err.has_value())
			return std::make_pair(p.first, err.value());
		return std::make_pair(p.first, std::optional<CompilationError>());
	}

	std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
		std::vector<Token> result;
		while (true) {
			auto p = NextToken();
			if (p.second.has_value()) {
				if (p.second.value().GetCode() == ErrorCode::ErrEOF)
					return std::make_pair(result, std::optional<CompilationError>());
				else
					return std::make_pair(std::vector<Token>(), p.second);
			}
			result.emplace_back(p.first.value());
		}
	}

	// 注意：这里的返回值中 Token 和 CompilationError 只能返回一个，不能同时返回。
	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
		// 用于存储已经读到的组成当前token字符
		std::stringstream ss;
		// 分析token的结果，作为此函数的返回值
		std::pair<std::optional<Token>, std::optional<CompilationError>> result;
		// <行号，列号>，表示当前token的第一个字符在源代码中的位置
		std::pair<int64_t, int64_t> pos;
		// 记录当前自动机的状态，进入此函数时是初始状态
		DFAState current_state = DFAState::INITIAL_STATE;
		// 这是一个死循环，除非主动跳出
		// 每一次执行while内的代码，都可能导致状态的变更
		while (true) {
			// 读一个字符，请注意auto推导得出的类型是std::optional<char>
			// 这里其实有两种写法
			// 1. 每次循环前立即读入一个 char
			// 2. 只有在可能会转移的状态读入一个 char
			// 因为我们实现了 unread，为了省事我们选择第一种
			auto current_char = nextChar();
			// 针对当前的状态进行不同的操作
			switch (current_state) {
				// 初始状态
			case INITIAL_STATE: {
				// 已经读到了文件尾
				if (!current_char.has_value())
					// 返回一个空的token，和编译错误ErrEOF：遇到了文件尾
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrEOF));

				// 获取读到的字符的值，注意auto推导出的类型是char
				auto ch = current_char.value();
				// 标记是否读到了不合法的字符，初始化为否
				auto invalid = false;

				// 使用了自己封装的判断字符类型的函数，定义于 tokenizer/utils.hpp
				// see https://en.cppreference.com/w/cpp/string/byte/isblank
				if (cc0::isspace(ch)) // 读到的字符是空白字符（空格、换行、制表符等）
					current_state = DFAState::INITIAL_STATE; // 保留当前状态为初始状态，此处直接break也是可以的
				else if (!cc0::isprint(ch)) // control codes and backspace
					invalid = true;
				else if (ch == '0') // 可能是0、十六进制或错误
				    current_state = DFAState::INTEGER_STATE;
				else if (cc0::isdigit(ch)) // 非零数字
					current_state = DFAState::DECIMAL_INTEGER_STATE; // 切换到十进制整数的状态
				else if (cc0::isalpha(ch)) // 读到的字符是英文字母
					current_state = DFAState::IDENTIFIER_STATE; // 切换到标识符的状态
				else {
					switch (ch) {
					case '=':
						current_state = DFAState::EQUAL_SIGN_STATE;
						break;
					case '-':
						current_state = DFAState::MINUS_SIGN_STATE;
						break;
					case '+':
						current_state = DFAState::PLUS_SIGN_STATE;
						break;
					case '*':
						current_state = DFAState::MULTIPLICATION_SIGN_STATE;
						break;
					case '/':
						current_state = DFAState::DIVISION_SIGN_STATE;
						break;
					case ';':
					    current_state = DFAState::SEMICOLON_STATE;
					    break;
                    case '(':
                        current_state = DFAState::LEFTBRACKET_STATE;
                        break;
                    case ')':
                        current_state = DFAState::RIGHTBRACKET_STATE;
                        break;
                    case ',':
                        current_state = DFAState::COMMA_SIGN_STATE;
                        break;
                    case '{':
                        current_state = DFAState::LEFT_BRACE_STATE;
                        break;
                    case '}':
                        current_state = DFAState::RIGHT_BRACE_STATE;
                        break;
                    case '<':
                        current_state = DFAState::LESS_SIGN_STATE;
                        break;
                    case '>':
                        current_state = DFAState::GREATER_SIGN_STATE;
                        break;
                    case '!':
                        current_state = DFAState::EXCLAMATION_SIGN_STATE;
                        break;
                    case '\'':
                        current_state = DFAState::CHAR_STATE;
                        break;
                    case '\"':
                        current_state = DFAState::STRING_STATE;
                        break;
					// 不接受的字符导致的不合法的状态
					default:
						invalid = true;
						break;
					}
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE)
					pos = previousPos(); // 记录该字符的的位置为token的开始位置
				// 读到了不合法的字符
				if (invalid) {
					// 回退这个字符
					unreadLast();
					// 返回编译错误：非法的输入
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
					ss << ch; // 存储读到的字符
				break;
			}

			// 开头是0就切换到这里
			case INTEGER_STATE: {
			    if(!current_char.has_value()) { // 理论上这里就是一位数字：0
                    std::string str = ss.str();
                    try {
                        return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(str), pos, currentPos()), std::optional<CompilationError>());
                    } catch (const std::out_of_range&) { // 溢出
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                    }
			    }
			    auto ch = current_char.value();
			    if(ch == 'X' || ch == 'x') { // 0x | 0X，十六进制
			        ss << ch;
			        current_state = DFAState::HEXADECIMAL_INTEGER_STATE;
			    }
			    else if(cc0::isdigit(ch)) // 以0开头的十进制数，报错
			        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInteger));
			    else if(cc0::isalpha(ch)) { // 如果是其它字母，就切换到标识符状态
			        ss << ch;
			        current_state = DFAState::IDENTIFIER_STATE;
			    }
			    else {// 否则就直接输出这个0
			        unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(ss.str()), pos, currentPos()), std::optional<CompilationError>());
                }
			    break;
			}

			// 当前状态是十六进制整数（已经是 0X | 0x 开头）
			case HEXADECIMAL_INTEGER_STATE: {
                if(!current_char.has_value()) { // 读到文件尾
                    std::string str = ss.str();
                    if(str == "0x" || str == "0X")
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInteger));
                    try {
                        return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(str, NULL, 16), pos, currentPos()), std::optional<CompilationError>());
                    } catch (const std::invalid_argument&) {
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrHexademicalChange));
                    } catch (const std::out_of_range&) {
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                    }
                }
                auto ch = current_char.value();
                if(isHex(ch))
                    ss << ch;
                else if(cc0::isalpha(ch)) {
                    ss << ch;
                    current_state = DFAState::IDENTIFIER_STATE;
                }
                else {
                    unreadLast();
                    std::string str = ss.str();
                    if(str == "0x" || str == "0X")
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
                    try {
                        return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(str, NULL, 16), pos, currentPos()), std::optional<CompilationError>());
                    } catch (const std::invalid_argument&) {
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrHexademicalChange));
                    } catch (const std::out_of_range&) {
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                    }
                }
			    break;
			}

			// 当前状态是十进制整数，且开头不为0
			case DECIMAL_INTEGER_STATE: {
				// 如果当前已经读到了文件尾，则解析已经读到的字符串为整数
				//     解析成功则返回无符号整数类型的token，否则返回编译错误
				// 如果读到的字符是数字，则存储读到的字符
				// 如果读到的是字母，则存储读到的字符，并切换状态到标识符
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
				//     解析成功则返回无符号整数类型的token，否则返回编译错误
				if(!current_char.has_value()) { // 读到文件尾
                    std::string str = ss.str();
                    try {
                        return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(str), pos, currentPos()), std::optional<CompilationError>());
                    } catch (const std::out_of_range&) { // 溢出
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                    }
                }
				auto ch = current_char.value();
                if(cc0::isdigit(ch)) // 数字，存储
                    ss << ch;
                else if(cc0::isalpha(ch)) { //字母，存储并切换标识符状态
                    ss << ch;
                    current_state = DFAState::IDENTIFIER_STATE;
                }
                else {
                    unreadLast(); // 回退并转换为无符号整数
                    std::string str = ss.str();
                    try {
                        return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::stoi(str), pos, currentPos()), std::optional<CompilationError>());
                    } catch (const std::out_of_range&) {
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                    }
                }
				break;
			}

			// 标识符状态
			case IDENTIFIER_STATE: {
				// 请填空：
				// 如果当前已经读到了文件尾，则解析已经读到的字符串
				//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
				// 如果读到的是字符或字母，则存储读到的字符
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
				//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
				if(!current_char.has_value()) {
					std::string str = ss.str();
                    TokenType type;
					auto it = reservedKeys.find(str);
					if(it == reservedKeys.end())
						type = TokenType::IDENTIFIER;
					else
					    type = it->second;
					return std::make_pair(std::make_optional<Token>(type, str, pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				if(cc0::isalpha(ch) || cc0::isdigit(ch))
					ss << ch;
				else {
					unreadLast();
                    std::string str = ss.str();
                    TokenType type;
                    auto it = reservedKeys.find(str);
                    if(it == reservedKeys.end())
                        type = TokenType::IDENTIFIER;
                    else
                        type = it->second;
                    return std::make_pair(std::make_optional<Token>(type, str, pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}

			// <char-liter> ::= "'" (<c-char>|<escape-seq>) "'"
			// <string-literal> ::= '"' {<s-char>|<escape-seq>} '"'
            // <escape-seq> ::= '\\' | "\'" | '\"' | '\n' | '\r' | '\t' | '\x'<hexadecimal-digit><hexadecimal-digit>
            //<printable> ::= <expression> | <string-literal> | <char-literal>
            // 转义字符直接输出为 \x??  懒得改了。。
            // https://forum.lazymio.cn/t/266
			case CHAR_STATE: {
			    std::stringstream ss;
			    if(!current_char.has_value()) // 读到文件尾，不符合文法报错
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrCharInvalid));

			    // <c-char>|<escape-seq>
			    auto ch = current_char.value();
			    if(isCChar(ch))
			        ss << ch;
			    else if(ch == '\\') {
			        current_char = nextChar();
			        if(!current_char.has_value() || !isEscape(ss, current_char.value()))
                        return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrCharInvalid));
                }else
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrCharInvalid));

                // '
                current_char = nextChar();
                if(!current_char.has_value() || current_char.value() != '\'')
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrCharInvalid));
                return std::make_pair(std::make_optional<Token>(TokenType::CHAR_TOKEN, ss.str(), pos, currentPos()), std::optional<CompilationError>());
			}

			// <string-literal> ::= '"' {<s-char>|<escape-seq>} '"'
			case STRING_STATE: {
                if(!current_char.has_value())
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrStringInvalid));
                std::stringstream ss;
                char ch;
                while(true) {
                    ch = current_char.value();
                    if(isSChar(ch))
                        ss << ch;
                    else if(ch == '\\') {
                        current_char = nextChar();
                        if(!current_char.has_value() || !isEscape(ss, current_char.value()))
                            return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrStringInvalid));
                    } else
                        break;
                    current_char = nextChar();
                }
                if(ch != '\"')
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrStringInvalid));
                return std::make_pair(std::make_optional<Token>(TokenType::STRING, ss.str(), pos, currentPos()), std::optional<CompilationError>());
            }

			// 如果当前状态是加号
			case PLUS_SIGN_STATE: {
				// 请思考这里为什么要回退，在其他地方会不会需要
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()), std::optional<CompilationError>());
			}

			// 当前状态为减号的状态
			case MINUS_SIGN_STATE: {
				// 请填空：回退，并返回减号token
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()), std::optional<CompilationError>());
			}
			
			// 除号/、注释//、/* */
			case DIVISION_SIGN_STATE: {
			    // 此时 previous 为 '/'
			    if(!current_char.has_value()) {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()), std::optional<CompilationError>());
			    }
			    auto ch = current_char.value();
			    switch (ch) {
                case '/':
                    current_state = DFAState::SINGLE_LINE_COMMENT_STATE;
                    break;
                case '*':
                    current_state = DFAState::MULTI_LINE_COMMENT_STATE;
                    break;
                default:
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()), std::optional<CompilationError>());
			    }
			    break;
			}

			// 单行注释
			// <single-line-comment> ::= '//' {<any-char>} (<LF>|<CR>)    // <LF>是ascii值为0x0A的字符  <CR>是ascii值为0x0D的字符
			case SINGLE_LINE_COMMENT_STATE: {
                if(!current_char.has_value()) {
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIncompleteCommit));
                }
                auto ch = current_char.value();
                if(ch == 0x0A || ch == 0x0D) {  // 单行注释结束。注释不应该被词法分析输出
                    return nextToken();
                }
			    break;
			}

			// 多行注释
			case MULTI_LINE_COMMENT_STATE: {
			    if(!current_char.has_value()) {
                    return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIncompleteCommit));
			    }
			    auto ch = current_char.value();
			    if(ch == '*') {
			        ch = nextChar().value();
			        if(ch == '/') // 多行注释结束
                        return nextToken();
			    }
			    break;
			}

			// 当前状态为乘号状态
			case MULTIPLICATION_SIGN_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()), std::optional<CompilationError>());
			}

			// = ==
			case EQUAL_SIGN_STATE: {
                if(current_char.value()) {
                    auto ch = current_char.value();
                    if(ch == '=') {
                        std::any value{std::in_place_type<std::string>, "=="};
                        return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, value, pos, currentPos()), std::optional<CompilationError>());
                    }
                }
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::ASSIGN_SIGN, '=', pos, currentPos()), std::optional<CompilationError>());
			}
			
			// ;
			case SEMICOLON_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()), std::optional<CompilationError>());
			}
			
			// (
			case LEFTBRACKET_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()), std::optional<CompilationError>());
			}

			// )
			case RIGHTBRACKET_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()), std::optional<CompilationError>());
			}

			// {
			case LEFT_BRACE_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACE, '{', pos, currentPos()), std::optional<CompilationError>());
			}

			// }
			case RIGHT_BRACE_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACE, '}', pos, currentPos()), std::optional<CompilationError>());
			}

			// < <=
			case LESS_SIGN_STATE: {
                if(current_char.value()) {
                    auto ch = current_char.value();
                    if(ch == '=') { // <=
                        std::any value{std::in_place_type<std::string>, "<="};
                        return std::make_pair(std::make_optional<Token>(TokenType::LESS_EQUAL_SIGN, value, pos, currentPos()), std::optional<CompilationError>());
                    }
                }
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::LESS_SIGN, '<', pos, currentPos()), std::optional<CompilationError>());
			}

            // > >=
			case GREATER_SIGN_STATE: {
                if(current_char.value()) {
                    auto ch = current_char.value();
                    if(ch == '=') {
                        std::any value{std::in_place_type<std::string>, ">="};
                        return std::make_pair(std::make_optional<Token>(TokenType::GREATER_EQUAL_SIGN, value, pos, currentPos()), std::optional<CompilationError>());
                    }
                }
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::GREATER_SIGN, '>', pos, currentPos()), std::optional<CompilationError>());
			}

			// ,
			case COMMA_SIGN_STATE: {
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::COMMA_SIGN, ',', pos, currentPos()), std::optional<CompilationError>());
			}

			// !=
			case EXCLAMATION_SIGN_STATE: {
                if(current_char.has_value()) {
                    auto ch = current_char.value();
                    if(ch == '=') {
                        std::any value{std::in_place_type<std::string>, "!="};
                        return std::make_pair(std::make_optional<Token>(TokenType::NONEQUAL_SIGN, value, pos, currentPos()), std::optional<CompilationError>());
                    }
                }
                return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
            }

			// 预料之外的状态，如果执行到了这里，说明程序异常
			default:
				DieAndPrint("unhandled state.");
				break;
            }
		}
		// 预料之外的状态，如果执行到了这里，说明程序异常
		return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
	}

	std::optional<CompilationError> Tokenizer::checkToken(const Token& t) {
		switch (t.GetType()) {
			case IDENTIFIER: {
				auto val = t.GetValueString();
				if (cc0::isdigit(val[0]))
					return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second, ErrorCode::ErrInvalidIdentifier);
				break;
			}
		default:
			break;
		}
		return {};
	}

	void Tokenizer::readAll() {
		if (_initialized)
			return;
		for (std::string tp; std::getline(_rdr, tp);)
			_lines_buffer.emplace_back(std::move(tp + "\n"));
		_initialized = true;
		_ptr = std::make_pair<int64_t, int64_t>(0, 0);
		return;
	}

	// Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
	std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
		if (_ptr.first >= _lines_buffer.size())
			DieAndPrint("advance after EOF");
		if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
			return std::make_pair(_ptr.first + 1, 0);
		else
			return std::make_pair(_ptr.first, _ptr.second + 1);
	}

	std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
		return _ptr;
	}

	std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
		if (_ptr.first == 0 && _ptr.second == 0)
			DieAndPrint("previous position from beginning");
		if (_ptr.second == 0)
			return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
		else
			return std::make_pair(_ptr.first, _ptr.second - 1);
	}

	std::optional<char> Tokenizer::nextChar() {
		if (isEOF())
			return {}; // EOF
		auto result = _lines_buffer[_ptr.first][_ptr.second];
		_ptr = nextPos();
		return result;
	}

	bool Tokenizer::isEOF() {
		return _ptr.first >= _lines_buffer.size();
	}

	bool Tokenizer::isHex(char ch) {
	    if(cc0::isdigit(ch))
            return true;
        if(ch >= 'a' && ch <= 'f')
            return true;
        if(ch >= 'A' && ch <= 'F')
            return true;
        return false;
	}

	/*
    4种空白符：空格（0x20, ' '）、水平制表符（0x09, '\t'）
    10种数字：从0到9
    52种英文字母：从a到z，从A到Z
    标点字符：_ ( ) [ ] { } < = > . , : ; ! ? + - * / % ^ & | ~  "  ` $ # @
	 */
	bool Tokenizer::isCChar(char ch) {
	    if(isdigit(ch) || isalpha(ch))
            return true;
	    if(ch == 0x20 || ch == 0x09)
            return true;
        switch(ch) {
        case '_': case '(': case ')': case '[': case ']': case '{': case '}':
        case '<': case '=': case '>': case '.': case ',': case ':': case ';':
        case '!': case '?': case '+': case '-': case '*': case '/': case '%':
        case '^': case '&': case '|': case '~': case '\"': case '`': case '$':
        case '#': case '@':
            return true;
            default:
                return false;
        }
	}

    bool Tokenizer::isSChar(char ch) {
        if(isdigit(ch) || isalpha(ch))
            return true;
        if(ch == 0x20 || ch == 0x09)
            return true;
        switch(ch) {
            case '_': case '(': case ')': case '[': case ']': case '{': case '}':
            case '<': case '=': case '>': case '.': case ',': case ':': case ';':
            case '!': case '?': case '+': case '-': case '*': case '/': case '%':
            case '^': case '&': case '|': case '~': case '\'': case '`': case '$':
            case '#': case '@':
                return true;
            default:
                return false;
        }
    }

    // <escape-seq> ::= '\\' | "\'" | '\"' | '\n' | '\r' | '\t' | '\x'<hexadecimal-digit><hexadecimal-digit>
    bool Tokenizer::isEscape(std::stringstream& ss, char ch) {
	    char res;
	    if(ch == 'x') {
	        std::stringstream tmp;
	        tmp << '0';
	        tmp << 'x';
	        auto hex = nextChar();
	        if(!hex.has_value() || !isHex(hex.value()))
                return false;
            tmp << hex.value();
            hex = nextChar();
            if(!hex.has_value() || !isHex(hex.value()))
                return false;
            tmp << hex.value();
            char a = std::stoi(tmp.str(), NULL, 16);
            ss << a;
            return true;
	    }
	    bool flag = true;
        switch(ch) {
            case '\\':
                res = '\\';
                break;
            case '\'':
                res = '\'';
                break;
            case '\"':
                res = '\"';
                break;
            case 'n':
                res = '\n';
                break;
            case 'r':
                res = '\r';
                break;
            case 't':
                res = '\t';
                break;
            default:
                flag = false;
                break;
        }
        if(flag)
            ss << res;
        return flag;
	}

	// Note: Is it evil to unread a buffer?
	void Tokenizer::unreadLast() {
		_ptr = previousPos();
	}
}