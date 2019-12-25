#include "argparse.hpp"
#include "fmt/core.h"

#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "fmts.hpp"
#include "main.h"

#include <iostream>
#include <fstream>

std::vector<cc0::Token> _tokenize(std::istream& input) {
	cc0::Tokenizer tkz(input);
	auto p = tkz.AllTokens();
	if (p.second.has_value()) {
		fmt::print(stderr, "Tokenization error: {}\n", p.second.value());
		exit(2);
	}
	return p.first;
}

void Tokenize(std::istream& input, std::ostream& output) {
	auto v = _tokenize(input);
	for (auto& it : v)
		output << fmt::format("{}\n", it);
	return;
}

void ToAssembly(std::istream& input, std::ostream& output){
	auto tks = _tokenize(input);
	// 打印词法分析输出结果
//    for (auto& it : tks)
//        output << fmt::format("{}\n", it);
	cc0::Analyser analyser(tks);
	auto p = analyser.Analyse();
//	analyser.printSym();
	if (p.second.has_value()) {
		fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
		exit(2);
	}
	// 输出常量表
	auto consts = analyser.getConstants();
	auto const_size = consts.size();
	output << ".constants:" << std::endl;
	for(int i=0; i<const_size; i++) {
	    //          下标  常量的类型       常量的值
	    output << i << " S \"" << consts[i].getName() << "\"" << std::endl;
	    // 这里其实就不让数字放在常量表了
	}

    // 输出启动代码
	auto v = p.first;
	output << ".start:" << std::endl;
	auto size = v[-1].size();
	for (int i=0; i<size; i++)
		output << fmt::format("{}   {}\n", i, v[-1][i]);

	// int32_t funcs_size = analyser.getFuncSize();  // v.size()-1
	// std::cout << funcs_size << " = " << v.size()-1 << std::endl;
    // 输出函数表
    output << ".functions:" << std::endl;
    int funcIndex = 0;
    for(int i=0; i<const_size; i++) {
        if(consts[i].isFunction())
            //          下标 函数名在.constants中的下标 参数占用的slot数 函数嵌套的层级
            output << funcIndex++ << " " << i << " " << consts[i].getParamNum() << " 1" << std::endl;
    }

    funcIndex = 0;
    for(int i=0; i<const_size; i++) {
        // 注意函数在const的位置i就是函数指令在vector的位置
        if(consts[i].isFunction()) {
            output << ".F" << funcIndex << ":" << std::endl;
            funcIndex++;
            // auto index = consts[i].getIndex(); 就是 i
            auto size = v[i].size();
            for(int j=0; j<size; j++)
                output << fmt::format("{}   {}\n", j, v[i][j]);
        }
    }

	return;
}

void writeBytes(void* addr, int count, std::ostream& out) {
    char bytes[8];
    assert(0 < count && count <= 8);
    char* p = reinterpret_cast<char*>(addr) + (count-1);
    for(int i=0; i<count; i++)
        bytes[i] = *p--;
    out.write(bytes, count);
}

void ToBinary(std::istream& input, std::ostream& out) {
    auto tks = _tokenize(input);
    cc0::Analyser analyser(tks);
    auto p = analyser.Analyse();
//	analyser.printSym();
    if (p.second.has_value()) {
        fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
        exit(2);
    }
    // 获取常量表
    auto consts = analyser.getConstants();

    // 输出 magic
    out.write("\x43\x30\x3A\x29", 4);
    // 输出 version
    out.write("\x00\x00\x00\x01", 4);

    // constants_count
    cc0::u2 constants_count = consts.size();
    writeBytes(&constants_count, sizeof(constants_count), out);
    // constants
    for(auto& constant: consts) {
        // 字符串常量（函数、字符串字面量）
        if(constant.isFunction() || constant.getType()==cc0::SymType::STRING_TYPE) {
            out.write("\x00", 1);
            std::string str = constant.getName();
            cc0::u2 len = str.length();
            // 输出字符串长度
            writeBytes(&len, sizeof(len), out);
            // 再输出字符串内容
            out.write(str.c_str(), len);
        } else if(constant.getType() == cc0::SymType::INT_TYPE) {
            // 整数常量，其实还没有这个东西
            out.write("\x01", 1);
            // 那就不写了
        } else if(constant.getType() == cc0::SymType::DOUBLE_TYPE) {
            // 浮点数常量，其实也没实现
            out.write("\x02", 1);
            // 等实现了再说
        }
    }

    auto to_binary = [&](const std::vector<cc0::Instruction>& v) {
        // u2 instructions_count;
        cc0::u2 intro_size = v.size();
        writeBytes(&intro_size, sizeof(intro_size), out);
        // Instruction instructions[instructions_count];
        for(auto& intro: v) {
            // 输出指令
            cc0::u1 opt = static_cast<cc0::u1>(intro.getOperation());
            writeBytes(&opt, sizeof(opt), out);
            // 指令后有没有参数
            auto iter = cc0::paramOpt.find(intro.getOperation());
            if(iter != cc0::paramOpt.end()) {
                auto params = iter->second;
                switch(params[0]) {
                    case 1: {
                        cc0::u1 x = intro.getX();
                        writeBytes(&x, 1, out);
                        break;
                    }
                    case 2: {
                        cc0::u2 x = intro.getX();
                        writeBytes(&x, 2, out);
                        break;
                    }
                    case 4: {
                        cc0::u4 x = intro.getX();
                        writeBytes(&x, 4, out);
                        break;
                    }
                    default:
                        break;
                }
                if(params.size() == 2) {
                    switch(params[1]) {
                        case 1: {
                            cc0::u1 y = intro.getY();
                            writeBytes(&y, 1, out);
                            break;
                        }
                        case 2: {
                            cc0::u2 y = intro.getY();
                            writeBytes(&y, 2, out);
                            break;
                        }
                        case 4: {
                            cc0::u4 y = intro.getY();
                            writeBytes(&y, 4, out);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    };

    // 指令全在这里: 启动代码、函数指令
    auto introductions_code = p.first;
    // start_code
    auto start_code = introductions_code[-1];
    to_binary(start_code);

    // functions_count
    cc0::u2 functions_count = introductions_code.size() - 1;
    writeBytes(&functions_count, sizeof(functions_count), out);

    // functions
    for(int i=0; i<constants_count; i++) {
        // 注意常量和函数是放在一起的
        if(consts[i].isFunction()) {
            // u2 name_index; // name: CO_binary_file.strings[name_index]
            cc0::u2 funcIndex = i;
            writeBytes(&funcIndex, sizeof(funcIndex), out);
            // u2 params_size;
            cc0::u2 paramSize = consts[i].getParamNum();
            writeBytes(&paramSize, sizeof(paramSize), out);
            // u2 level;
            cc0::u2 level = 1;
            writeBytes(&level, sizeof(level), out);
            to_binary(introductions_code[i]);
        }
    }
    return;
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("cc0");
	program.add_argument("input")
		.help("speicify the file to be compiled.");
	program.add_argument("-c")
		.default_value(false)
		.implicit_value(true)
		.help("translate to binary file for the input file.");
	program.add_argument("-s")
		.default_value(false)
		.implicit_value(true)
		.help("translate to assembly file for the input file.");
	program.add_argument("-o", "--output")
		.required()
		.default_value(std::string("-"))
		.help("specify the output file.");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		fmt::print(stderr, "{}\n\n", err.what());
		program.print_help();
		exit(2);
	}

	auto input_file = program.get<std::string>("input");
	auto output_file = program.get<std::string>("--output");
	std::istream* input;
	std::ostream* output;
	std::ifstream inf;
	std::ofstream outf;
	if (input_file != "-") {
		inf.open(input_file, std::ios::in);
		if (!inf) {
			fmt::print(stderr, "Fail to open {} for reading.\n", input_file);
			exit(2);
		}
		input = &inf;
	}
	else
		input = &std::cin;
	if (output_file != "-") {
		outf.open(output_file, std::ios::out | std::ios::trunc);
		if (!outf) {
			fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
			exit(2);
		}
		output = &outf;
	}
	else
		output = &std::cout;
	if (program["-c"] == true && program["-s"] == true) {
		fmt::print(stderr, "You can only perform tokenization or syntactic analysis at one time.");
		exit(2);
	}
	if (program["-c"] == true) {
	    // 生成二进制
		ToBinary(*input, *output);
	}
	else if (program["-s"] == true) {
		ToAssembly(*input, *output);
	}
	else {
		fmt::print(stderr, "You must choose tokenization or syntactic analysis.");
		exit(2);
	}
	return 0;
}