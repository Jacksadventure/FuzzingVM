#ifndef TOKENTHREADING_H
#define TOKENTHREADING_H

#include <vector>
#include <stack>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include "symbol.hpp"
#include "readfile.hpp"
#include "interface.hpp"
#ifdef _WIN32
#include <windows.h> // Windows-specific headers for file operations
#endif

class DirectThreadingVM : public Interface {
    uint32_t ip; // Instruction pointer
    std::vector<std::stack<uint32_t>> sts; // Stack for operations
    std::stack<uint32_t> st;
    std::vector<uint32_t> instructions; // Instruction set
    char* buffer; // Memory buffer
    std::stack<uint32_t> callStack; // Call stack for function calls

    inline float to_float(uint32_t val) {
        return *reinterpret_cast<float*>(&val);
    }
    inline uint32_t from_float(float val) {
        return *reinterpret_cast<uint32_t*>(&val);
    }
    inline void write_memory(char* buffer, uint32_t* src, uint32_t offset, uint32_t size) {
        memcpy(buffer + offset, src, size);
    }
    inline void write_mem32(char* buffer, uint32_t val, uint32_t offset) {
        uint32_t buf[1] = { val };
        write_memory(buffer, buf, offset, 4);
    }
    inline void read_memory(char* buffer, uint8_t* dst, uint32_t offset, uint32_t size) {
        memcpy(dst, buffer + offset, size);
    }
    inline uint32_t read_mem32(char* buffer, uint32_t offset) {
        uint32_t buf[1];
        read_memory(buffer, reinterpret_cast<uint8_t*>(buf), offset, 4);
        return buf[0];
    }

    // Define operations for each instruction
    inline void do_add() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(a + b);
    }
    inline void do_sub() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b - a);
    }
    inline void do_mul() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(a * b);
    }
    inline void do_div() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        if (b == 0) {
            std::cerr << "Error: Divided by zero error" << std::endl;
            return;
        }
        st.push(b / a);
    }
    inline void do_fp_add() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        st.push(from_float(a + b));
    }
    inline void do_fp_sub() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        st.push(from_float(b - a));
    }
    inline void do_fp_mul() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        st.push(from_float(a * b));
    }
    inline void do_fp_div() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        if (b == 0.0f) {
            std::cerr << "Division by zero error" << std::endl;
            return;
        }
        st.push(from_float(b / a));
    }
    inline void do_inc() {
        uint32_t a = st.top(); st.pop();
        st.push(++a);
    }
    inline void do_dec() {
        uint32_t a = st.top(); st.pop();
        st.push(--a);
    }
    inline void do_shl() {
        uint32_t shift = st.top(); st.pop();
        uint32_t value = st.top(); st.pop();
        st.push(value << shift);
    }
    inline void do_shr() {
        uint32_t shift = st.top(); st.pop();
        uint32_t value = st.top(); st.pop();
        st.push(value >> shift);
    }
    inline void do_end() {
        st = std::stack<uint32_t>();
        instructions = std::vector<uint32_t>();
        ip = 0;
    }
    inline void do_lod() {
        uint32_t offset = instructions[++ip];
        uint32_t a = read_mem32(buffer, offset);
        st.push(a);
    }
    inline void do_sto() {
        uint32_t offset = instructions[++ip];
        uint32_t a = st.top(); st.pop();
        write_mem32(buffer, a, offset);
    }
    inline void do_immi() {
        uint32_t a = instructions[++ip];
        st.push(a);
    }
    inline void do_memcpy() {
        uint32_t dest = instructions[++ip];
        uint32_t src = instructions[++ip];
        uint32_t len  = instructions[++ip];
        memcpy(buffer + dest, buffer + src, len);
    }
    inline void do_memset() {
        uint32_t dest = instructions[++ip];
        uint32_t val  = instructions[++ip];
        uint32_t len  = instructions[++ip];
        memset(buffer + dest, val, len);
    }
    inline void do_sto_immi() {
        uint32_t offset = instructions[++ip];
        uint32_t number = instructions[++ip];
        write_mem32(buffer, number, offset);
    }
    inline void do_jmp() {
        uint32_t target = instructions[++ip];
        ip = target - 1;
    }
    inline void do_jz() {
        uint32_t target = instructions[++ip];
        if (st.top() == 0) {
            ip = target - 1;
        }
        st.pop();
    }
    inline void do_jump_if() {
        uint32_t condition = st.top(); st.pop();
        uint32_t target = instructions[++ip];
        if (condition) {
            ip = target - 1;
        }
    }
    inline void do_if_else() {
        uint32_t condition = st.top(); st.pop();
        uint32_t trueBranch = instructions[++ip];
        uint32_t falseBranch = instructions[++ip];
        ip = condition ? trueBranch - 1 : falseBranch - 1;
    }
    inline void do_gt() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b > a ? 1 : 0);
    }
    inline void do_lt() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b < a ? 1 : 0);
    }
    inline void do_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b == a ? 1 : 0);
    }
    inline void do_gt_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b >= a ? 1 : 0);
    }
    inline void do_lt_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b <= a ? 1 : 0);
    }
    inline void do_call() {
        uint32_t target = instructions[++ip];
        uint32_t num_params = instructions[++ip];
        std::stack<uint32_t> newStack;
        for (uint32_t i = 0; i < num_params; ++i) {
            newStack.push(st.top());
            st.pop();
        }
        sts.push_back(newStack);
        st = sts.back(); // st always refers to the top of stacks.
        callStack.push(ip);
        ip = target - 1;
    }
    inline void do_ret() {
        if (callStack.empty()) {
            std::cerr << "Error: Call stack underflow" << std::endl;
            return;
        }
        uint32_t return_value = st.top();
        ip = callStack.top(); callStack.pop();
        sts.pop_back();
        st = sts.back();
        st.push(return_value);
    }
    inline void do_seek() {
        debug_num = st.top();
    }
    inline void do_print() {
        if (!st.empty()) {
            std::cout << static_cast<int>(st.top()) << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
    }
    inline void do_print_fp() {
        if (!st.empty()) {
            uint32_t num = st.top();
            float* floatPtr = reinterpret_cast<float*>(&num);
            std::cout << *floatPtr << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
    }
    inline void do_read_fp() {
        uint32_t offset = instructions[++ip];
        float val;
        std::cin >> val;
        write_mem32(buffer, from_float(val), offset);
    }
    inline void do_read_int() {
        uint32_t offset = instructions[++ip];
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
    }
    inline void tik() {
        std::cout << "tik" << std::endl;
    }

public:
    uint32_t debug_num;
    DirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]), debug_num(0xFFFFFFFF) {
        // The original function pointer table is no longer used in the dispatch loop.
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }
    ~DirectThreadingVM() {
        delete[] buffer;
    }

    // Modified run_vm method using computed goto dispatch.
    void run_vm(std::vector<uint32_t>& code) {
        instructions = code;
        ip = 0;

        // Macro to dispatch to the next instruction.
        #define DISPATCH() \
            if (ip < instructions.size()) { \
                goto *jumpTable[instructions[ip++]]; \
            } else { \
                goto exit_vm; \
            }

        // Initialize the jump table using the instruction codes (assumed to be defined in symbol.hpp)
        static void* jumpTable[256] = {
            [DT_ADD]      = &&inst_add,
            [DT_SUB]      = &&inst_sub,
            [DT_MUL]      = &&inst_mul,
            [DT_DIV]      = &&inst_div,
            [DT_SHL]      = &&inst_shl,
            [DT_SHR]      = &&inst_shr,
            [DT_FP_ADD]   = &&inst_fp_add,
            [DT_FP_SUB]   = &&inst_fp_sub,
            [DT_FP_MUL]   = &&inst_fp_mul,
            [DT_FP_DIV]   = &&inst_fp_div,
            [DT_END]      = &&inst_end,
            [DT_LOD]      = &&inst_lod,
            [DT_STO]      = &&inst_sto,
            [DT_IMMI]     = &&inst_immi,
            [DT_STO_IMMI] = &&inst_sto_immi,
            [DT_MEMCPY]   = &&inst_memcpy,
            [DT_MEMSET]   = &&inst_memset,
            [DT_INC]      = &&inst_inc,
            [DT_DEC]      = &&inst_dec,
            [DT_JMP]      = &&inst_jmp,
            [DT_JZ]       = &&inst_jz,
            [DT_IF_ELSE]  = &&inst_if_else,
            [DT_JUMP_IF]  = &&inst_jump_if,
            [DT_GT]       = &&inst_gt,
            [DT_LT]       = &&inst_lt,
            [DT_EQ]       = &&inst_eq,
            [DT_GT_EQ]    = &&inst_gt_eq,
            [DT_LT_EQ]    = &&inst_lt_eq,
            [DT_CALL]     = &&inst_call,
            [DT_RET]      = &&inst_ret,
            [DT_SEEK]     = &&inst_seek,
            [DT_PRINT]    = &&inst_print,
            [DT_READ_INT] = &&inst_read_int,
            [DT_FP_PRINT] = &&inst_fp_print,
            [DT_FP_READ]  = &&inst_fp_read,
            [DT_Tik]      = &&inst_Tik,
        };

        DISPATCH();

    inst_add:
        do_add();
        DISPATCH();

    inst_sub:
        do_sub();
        DISPATCH();

    inst_mul:
        do_mul();
        DISPATCH();

    inst_div:
        do_div();
        DISPATCH();

    inst_shl:
        do_shl();
        DISPATCH();

    inst_shr:
        do_shr();
        DISPATCH();

    inst_fp_add:
        do_fp_add();
        DISPATCH();

    inst_fp_sub:
        do_fp_sub();
        DISPATCH();

    inst_fp_mul:
        do_fp_mul();
        DISPATCH();

    inst_fp_div:
        do_fp_div();
        DISPATCH();

    inst_end:
        do_end();
        DISPATCH();

    inst_lod:
        do_lod();
        DISPATCH();

    inst_sto:
        do_sto();
        DISPATCH();

    inst_immi:
        do_immi();
        DISPATCH();

    inst_sto_immi:
        do_sto_immi();
        DISPATCH();

    inst_memcpy:
        do_memcpy();
        DISPATCH();

    inst_memset:
        do_memset();
        DISPATCH();

    inst_inc:
        do_inc();
        DISPATCH();

    inst_dec:
        do_dec();
        DISPATCH();

    inst_jmp:
        do_jmp();
        DISPATCH();

    inst_jz:
        do_jz();
        DISPATCH();

    inst_if_else:
        do_if_else();
        DISPATCH();

    inst_jump_if:
        do_jump_if();
        DISPATCH();

    inst_gt:
        do_gt();
        DISPATCH();

    inst_lt:
        do_lt();
        DISPATCH();

    inst_eq:
        do_eq();
        DISPATCH();

    inst_gt_eq:
        do_gt_eq();
        DISPATCH();

    inst_lt_eq:
        do_lt_eq();
        DISPATCH();

    inst_call:
        do_call();
        DISPATCH();

    inst_ret:
        do_ret();
        DISPATCH();

    inst_seek:
        do_seek();
        DISPATCH();

    inst_print:
        do_print();
        DISPATCH();

    inst_read_int:
        do_read_int();
        DISPATCH();

    inst_fp_print:
        do_print_fp();
        DISPATCH();

    inst_fp_read:
        do_read_fp();
        DISPATCH();

    inst_Tik:
        tik();
        DISPATCH();

    exit_vm:
        return;

        #undef DISPATCH
    }

    // Overload that reads the file and then runs the VM using computed goto.
    void run_vm(std::string filename, bool benchmarkMode) {
        try {
            instructions = readFileToUint32Array(filename);
            if (benchmarkMode) {
                std::cout << "Preprocessing completed, starting benchmark..." << std::endl;
            }
            run_vm(instructions);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};

#endif // TOKENTHREADING_H