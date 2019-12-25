#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace cc0 {

	enum Operation {
	    NOP     = 0x00,    // 什么都不做
	    BIPUSH  = 0x01,    // 将 byte 单字节提升至 int 值压栈
	    IPUSH   = 0x02,    // 压栈 int
	    POP     = 0x04,    // 弹出 1 个 slot
 	    POP2    = 0x05,    // 弹出 2 个 slot
	    POPN    = 0x06,    // 弹出 n 个 slot
	    DUP     = 0x07,    // 复制栈顶的 1 个 slot 并入栈
	    DUP2    = 0x08,
	    LOADC   = 0x09,    // 加载常量池下标为 index 的常量值 value
	    LOADA   = 0x0a,    // 沿 SL 链向前移动 level_diff 次（移动到当前栈帧层次差为 level_diff 的栈帧中），加载该栈帧中栈偏移为 offset 的内存的栈地址值 address
	    NEW     = 0x0b,    // 弹出栈顶的 int 值 count，在堆上分配连续的大小为 count 个 slot 的内存，然后将这段内存的首地址 address 压入栈。内存的值保证被初始化为 0。
        SNEW    = 0x0c,    // 在栈顶连续分配大小为 count 个 slot 的内存。内存的值不保证被初始化为 0
        ILOAD   = 0x10,    // 从内存地址 address 处加载一个 int 值
        DLOAD   = 0x11,    // double, 同上
        ALOAD   = 0x12,    // 从内存地址 address 处加载一个地址值
        IALOAD  = 0x18,    // 数组用的
        DALOAD  = 0x19,
        AALOAD  = 0x1a,
        ISTORE  = 0x20,
        DSTORE  = 0x21,
        ASTORE  = 0x22,
        IASTORE = 0x28,
        DASTORE = 0x29,
        AASTORE = 0x2a,
        IADD    = 0x30,
        DADD    = 0x31,
        ISUB    = 0x34,
        DSUB    = 0x35,
        IMUL    = 0x38,
        DMUL    = 0X39,
        IDIV    = 0x3c,
        DDIV    = 0x3d,
        INEG    = 0x40,    // 弹出栈顶 value，将 -value 的值 result 压栈
        DNEG    = 0x41,
        ICMP    = 0x44,    // 弹出栈顶 rhs 和次栈顶 lhs，并将比较结果以 int 值 result 压栈
        DCMP    = 0x45,
        I2D     = 0x60,    // int -> double
        D2I     = 0x61,
        I2C     = 0x62,
        JMP     = 0x70,
        JE      = 0x71,
        JNE     = 0x72,
        JL      = 0x73,
        JGE     = 0x74,
        JG      = 0x75,
        JLE     = 0x76,
        CALL    = 0x80,
        RET     = 0x88,
        IRET    = 0x89,
        DRET    = 0x8a,
        ARET    = 0x8b,
        IPRINT  = 0xa0,
        DPRINT  = 0xa1,
        CPRINT  = 0xa2,
        SPRINT  = 0xa3,
        PRINTL  = 0xaf,    // 换行
        ISCAN   = 0xB0,
        DSCAN   = 0xb1,
        CSCAN   = 0xb2
	};

	const std::unordered_map<cc0::Operation, std::vector<int>> paramOpt = {
	        // bipush byte(1) (0x01)
            {Operation::BIPUSH, {1} },
            // ipush value(4) (0x02)
            {Operation::IPUSH, {4} },
            // popn count(4) (0x06)
            {Operation::POPN, {4} },
            // loadc index(2) (0x09)
            {Operation::LOADC, {2} },
            // loada level_diff(2), offset(4) (0x0a)
            {Operation::LOADA, {2, 4} },
            // snew count(4) (0x0c)
            {Operation::SNEW, {4} },
            // call index(2) (0x80)
            {Operation::CALL, {2} },
            // jmp offset(2) (0x70)
            {Operation::JMP, {2} },
            // je offset(2) (0x71)
            {Operation::JE, {2} },
            {Operation::JNE, {2} },
            {Operation::JL, {2} },
            {Operation::JLE, {2} },
            {Operation::JG, {2} },
            {Operation::JGE, {2} },
	};
	
	class Instruction final {
	private:
        using int32_t = std::int32_t;
	public:
		friend void swap(Instruction& lhs, Instruction& rhs);
	public:
	    Instruction(Operation opr) : _opr(opr) {}
	    Instruction(Operation opr, int32_t x) : _opr(opr), _x(x) {}
		Instruction(Operation opr, int32_t x,int32_t y) : _opr(opr), _x(x), _y(y) {}

		Instruction() = default;
		Instruction(const Instruction& i) { _opr = i._opr; _x = i._x; _y = i._y; }
		Instruction(Instruction&& i) :Instruction() { swap(*this, i); }
		Instruction& operator=(Instruction i) { swap(*this, i); return *this; }
		bool operator==(const Instruction& i) const { return _opr == i._opr && _x == i._x && _y == i._y; }

		Operation getOperation() const { return _opr; }
		int32_t getX() const { return _x; }
		int32_t getY() const { return _y; }
		void setX(int32_t x) { _x = x; }
	private:
		Operation _opr;
		int32_t _x = -1;
		int32_t _y = -1;
	};

	inline void swap(Instruction& lhs, Instruction& rhs) {
		using std::swap;
		swap(lhs._opr, rhs._opr);
		swap(lhs._x, rhs._x);
		swap(lhs._y, rhs._y);
	}
}