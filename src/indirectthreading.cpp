#ifndef INDIRECTTHREADING_H
#define INDIRECTTHREADING_H

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
#include <stdlib.h>

class IndirectThreadingVM : public Interface {
    uint32_t ip; // Instruction pointer (used for compatibility with inline functions)
    std::vector<std::stack<uint32_t>> sts; // Stack for operations (for function calls)
    std::stack<uint32_t> st;             // Primary operand stack
    std::vector<uint32_t> instructions;  // Instruction set
    char* buffer;                        // Memory buffer
    void (IndirectThreadingVM::*instructionTable[256])(void); // (Unused in computed goto version)
    std::stack<uint32_t> callStack;        // Call stack for function calls

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
        uint32_t buf[1];
        buf[0] = val;
        write_memory(buffer, (uint32_t *)buf, offset, 4);
    }

    inline void read_memory(char* buffer, uint8_t* dst, uint32_t offset, uint32_t size) {
        memcpy(dst, buffer + offset, size);
    }

    inline uint32_t read_mem32(char* buffer, uint32_t offset) {
        uint32_t buf[1];
        read_memory(buffer, (uint8_t *)buf, offset, 4);
        return buf[0];
    }

    // Arithmetic operations (integer and floating point)
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
        if (a == 0) {
            std::cerr << "Error: Divided by zero error" << std::endl;
            return;
        }
        st.push(b / a);
    }

    inline void do_fp_add() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = a + b;
        st.push(from_float(result));
    }

    inline void do_fp_sub() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = b - a;
        st.push(from_float(result));
    }

    inline void do_fp_mul() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = a * b;
        st.push(from_float(result));
    }

    inline void do_fp_div() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        if (a == 0.0f) {
            std::cerr << "Division by zero error" << std::endl;
            return;
        }
        float result = b / a;
        st.push(from_float(result));
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

    // Memory operations
    // (The computed goto loop will reimplement these operations inline.)
    
    // Other operations
    inline void tik(){
        std::cout << "tik" << std::endl;
    }

    // (The original instructionTable initialization is no longer used in the computed goto version.)
    void init_instruction_table() {
        // This function can be left empty or removed since computed goto is used.
    }

public:
    uint32_t debug_num;
    IndirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) { 
        init_instruction_table();
        debug_num = 0xFFFFFFFF;
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }

    ~IndirectThreadingVM() {
        delete[] buffer;
    }

    // The filename-based run_vm now loads the instructions and then calls the computed goto version.
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

    // The main interpreter loop using computed gotos (Indirect threading)
    void run_vm(std::vector<uint32_t>& code) {
        instructions = code;
        // Pointer into our instruction array.
        uint32_t* iptr = instructions.data();

        // Build a dispatch table mapping opcodes to local labels.
        static void* dispatch[256] = {
            [DT_ADD]       = &&L_DT_ADD,
            [DT_SUB]       = &&L_DT_SUB,
            [DT_MUL]       = &&L_DT_MUL,
            [DT_DIV]       = &&L_DT_DIV,
            [DT_SHL]       = &&L_DT_SHL,
            [DT_SHR]       = &&L_DT_SHR,
            [DT_FP_ADD]    = &&L_DT_FP_ADD,
            [DT_FP_SUB]    = &&L_DT_FP_SUB,
            [DT_FP_MUL]    = &&L_DT_FP_MUL,
            [DT_FP_DIV]    = &&L_DT_FP_DIV,
            [DT_END]       = &&L_DT_END,
            [DT_LOD]       = &&L_DT_LOD,
            [DT_STO]       = &&L_DT_STO,
            [DT_IMMI]      = &&L_DT_IMMI,
            [DT_STO_IMMI]  = &&L_DT_STO_IMMI,
            [DT_MEMCPY]    = &&L_DT_MEMCPY,
            [DT_MEMSET]    = &&L_DT_MEMSET,
            [DT_INC]       = &&L_DT_INC,
            [DT_DEC]       = &&L_DT_DEC,
            [DT_JMP]       = &&L_DT_JMP,
            [DT_JZ]        = &&L_DT_JZ,
            [DT_JUMP_IF]   = &&L_DT_JUMP_IF,
            [DT_IF_ELSE]   = &&L_DT_IF_ELSE,
            [DT_GT]        = &&L_DT_GT,
            [DT_LT]        = &&L_DT_LT,
            [DT_EQ]        = &&L_DT_EQ,
            [DT_GT_EQ]     = &&L_DT_GT_EQ,
            [DT_LT_EQ]     = &&L_DT_LT_EQ,
            [DT_CALL]      = &&L_DT_CALL,
            [DT_RET]       = &&L_DT_RET,
            [DT_SEEK]      = &&L_DT_SEEK,
            [DT_PRINT]     = &&L_DT_PRINT,
            [DT_READ_INT]  = &&L_DT_READ_INT,
            [DT_FP_PRINT]  = &&L_DT_FP_PRINT,
            [DT_FP_READ]   = &&L_DT_FP_READ,
            [DT_Tik]       = &&L_DT_Tik
        };

        // Macro to jump to the next instruction.
#define NEXT goto *dispatch[*iptr++]

        NEXT;

    L_DT_ADD:
        ip = (iptr - instructions.data()) - 1;
        do_add();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_SUB:
        ip = (iptr - instructions.data()) - 1;
        do_sub();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_MUL:
        ip = (iptr - instructions.data()) - 1;
        do_mul();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_DIV:
        ip = (iptr - instructions.data()) - 1;
        do_div();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_SHL:
        ip = (iptr - instructions.data()) - 1;
        do_shl();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_SHR:
        ip = (iptr - instructions.data()) - 1;
        do_shr();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_FP_ADD:
        ip = (iptr - instructions.data()) - 1;
        do_fp_add();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_FP_SUB:
        ip = (iptr - instructions.data()) - 1;
        do_fp_sub();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_FP_MUL:
        ip = (iptr - instructions.data()) - 1;
        do_fp_mul();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_FP_DIV:
        ip = (iptr - instructions.data()) - 1;
        do_fp_div();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_END:
        ip = (iptr - instructions.data()) - 1;
        do_end();
        return;
    L_DT_LOD:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t offset = instructions[++ip];
        uint32_t a = read_mem32(buffer, offset);
        st.push(a);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_STO:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t offset = instructions[++ip];
        uint32_t a = st.top(); st.pop();
        write_mem32(buffer, a, offset);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_IMMI:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = instructions[++ip];
        st.push(a);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_STO_IMMI:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t offset = instructions[++ip];
        uint32_t number = instructions[++ip];
        write_mem32(buffer, number, offset);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_MEMCPY:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t dest = instructions[++ip];
        uint32_t src = instructions[++ip];
        uint32_t len = instructions[++ip];
        memcpy(buffer + dest, buffer + src, len);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_MEMSET:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t dest = instructions[++ip];
        uint32_t val = instructions[++ip];
        uint32_t len = instructions[++ip];
        memset(buffer + dest, val, len);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_INC:
        ip = (iptr - instructions.data()) - 1;
        do_inc();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_DEC:
        ip = (iptr - instructions.data()) - 1;
        do_dec();
        iptr = instructions.data() + ip + 1;
        NEXT;
    L_DT_JMP:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t target = instructions[++ip];
        ip = target - 1;
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_JZ:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t target = instructions[++ip];
        if (st.top() == 0) {
            ip = target - 1;
        }
        st.pop();
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_JUMP_IF:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t condition = st.top(); st.pop();
        uint32_t target = instructions[++ip];
        if (condition) {
            ip = target - 1;
        }
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_IF_ELSE:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t condition = st.top(); st.pop();
        uint32_t trueBranch = instructions[++ip];
        uint32_t falseBranch = instructions[++ip];
        ip = condition ? trueBranch - 1 : falseBranch - 1;
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_GT:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b > a ? 1 : 0);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_LT:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b < a ? 1 : 0);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_EQ:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b == a ? 1 : 0);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_GT_EQ:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b >= a ? 1 : 0);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_LT_EQ:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b <= a ? 1 : 0);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_CALL:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t target = instructions[++ip];
        uint32_t num_params = instructions[++ip];
        std::stack<uint32_t> newStack;
        for (uint32_t i = 0; i < num_params; ++i) {
            newStack.push(st.top());
            st.pop();
        }
        sts.push_back(newStack);
        st = sts.back();
        callStack.push(ip);
        ip = target - 1;
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_RET:
    {
        ip = (iptr - instructions.data()) - 1;
        if (callStack.empty()) {
            std::cerr << "Error: Call stack underflow" << std::endl;
            return;
        }
        uint32_t return_value = st.top();
        ip = callStack.top(); callStack.pop();
        sts.pop_back();
        st = sts.back();
        st.push(return_value);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_SEEK:
    {
        ip = (iptr - instructions.data()) - 1;
        debug_num = st.top();
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_PRINT:
    {
        ip = (iptr - instructions.data()) - 1;
        if (!st.empty()) {
            std::cout << (int)st.top() << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_READ_INT:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t offset = instructions[++ip];
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_FP_PRINT:
    {
        ip = (iptr - instructions.data()) - 1;
        if (!st.empty()) {
            uint32_t num = st.top();
            float* floatPtr = reinterpret_cast<float*>(&num);
            std::cout << *floatPtr << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_FP_READ:
    {
        ip = (iptr - instructions.data()) - 1;
        uint32_t offset = instructions[++ip];
        float val;
        std::cin >> val;
        write_mem32(buffer, from_float(val), offset);
        iptr = instructions.data() + ip + 1;
        NEXT;
    }
    L_DT_Tik:
    {
        ip = (iptr - instructions.data()) - 1;
        tik();
        iptr = instructions.data() + ip + 1;
        NEXT;
    }

#undef NEXT
    // End of computed goto loop.
    // (If execution reaches here, simply return.)
    return;
}
};
#endif // INDIRECTTHREADING_H


