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
    void (DirectThreadingVM::*instructionTable[256])(void); // Function pointer table for instructions
    std::stack<uint32_t> callStack; // Call stack for function calls
    float to_float(uint32_t val) {
        return *reinterpret_cast<float*>(&val);
    }

    uint32_t from_float(float val) {
        return *reinterpret_cast<uint32_t*>(&val);
    }

    void write_memory(char* buffer,uint32_t* src, uint32_t offset, uint32_t size) {
        memcpy(buffer + offset, src, size);
    }

    void write_mem32(char* buffer, uint32_t val, uint32_t offset) {
        uint32_t buf[1];
        buf[0] = val;
        write_memory(buffer, (uint32_t *)buf, offset, 4);
    }

    void read_memory(char* buffer, uint8_t* dst, uint32_t offset, uint32_t size) {
        memcpy(dst, buffer + offset, size);
    }

    uint32_t read_mem32(char* buffer, uint32_t offset) {
        uint32_t buf[1];
        read_memory(buffer, (uint8_t *)buf, offset, 4);
        return buf[0];
    }
    // Define operations for each instruction
    void do_add() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(a + b);
    }

    void do_sub() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b - a);
    }

    void do_mul() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(a * b);
    }

    void do_div() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        if (b == 0) {
            std::cerr << "Error: Divided by zero error" << std::endl;
            return;
        }
        st.push(b / a);
    }

    void do_fp_add() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = a + b;
        st.push(from_float(result));
    }

    void do_fp_sub() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = b - a;
        st.push(from_float(result));
    }

    void do_fp_mul() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        float result = a * b;
        st.push(from_float(result));
    }

    void do_fp_div() {
        float a = to_float(st.top()); st.pop();
        float b = to_float(st.top()); st.pop();
        if (b == 0.0f) {
            std::cerr << "Division by zero error" << std::endl;
            return;
        }
        float result = b / a;
        st.push(from_float(result));
    }

    void do_inc() {
        uint32_t a = st.top(); st.pop();
        st.push(++a);
    }

    void do_dec() {
        uint32_t a = st.top(); st.pop();
        st.push(--a);
    }

    void do_shl() {
        uint32_t shift = st.top(); st.pop();
        uint32_t value = st.top(); st.pop();
        st.push(value << shift);
    }

    void do_shr() {
        uint32_t shift = st.top(); st.pop();
        uint32_t value = st.top(); st.pop();
        st.push(value >> shift);
    }

    void do_end() {
        st = std::stack<uint32_t>();
        instructions = std::vector<uint32_t>();
        ip = 0;
    }

    void do_lod() {
        uint32_t offset = instructions[++ip];
        uint32_t a = read_mem32(buffer,offset);
        st.push(a);
    }

    void do_sto() {
        uint32_t offset = instructions[++ip];
        uint32_t a = st.top(); st.pop();
        write_mem32(buffer,a,offset);
    }

    void do_immi() {
        uint32_t a = instructions[++ip];
        st.push(a);
    }

    void do_memcpy() {
        uint32_t dest = instructions[++ip];
        uint32_t src = instructions[++ip];
        uint32_t len = instructions[++ip];
        memcpy(buffer + dest, buffer + src, len);
    }

    void do_memset() {
        uint32_t dest = instructions[++ip];
        uint32_t val = instructions[++ip];
        uint32_t len = instructions[++ip];
        memset(buffer + dest, val, len);
    }
    void do_sto_immi() {
        uint32_t offset = instructions[++ip];
        uint32_t number = instructions[++ip];
        write_mem32(buffer,number,offset);
    }

    void do_jmp() {
        uint32_t target = instructions[++ip];
        ip = target - 1;
    }

    void do_jz() {
        uint32_t target = instructions[++ip];
        if (st.top() == 0) {
            ip = target - 1;
        }
        st.pop();
    }

    void do_jump_if() {
        uint32_t condition = st.top(); st.pop();
        uint32_t target = instructions[++ip];
        if (condition) {
            ip = target - 1;
        }
    }

    void do_if_else() {
        uint32_t condition = st.top(); st.pop();
        uint32_t trueBranch = instructions[++ip];
        uint32_t falseBranch = instructions[++ip];
        ip = condition ? trueBranch - 1 : falseBranch - 1;
    }
    void do_gt() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b > a ? 1 : 0);
    }

    void do_lt() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b < a ? 1 : 0);
    }

    void do_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b == a ? 1 : 0);
    }

    void do_gt_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b >= a ? 1 : 0);
    }

    void do_lt_eq() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        st.push(b <= a ? 1 : 0);
    }

    void do_call() {
        uint32_t target = instructions[++ip]; 
        uint32_t num_params = instructions[++ip]; 
        std::stack<uint32_t> newStack;
         for (uint32_t i = 0; i < num_params; ++i) {
            newStack.push(st.top());
            st.pop();
        }
        sts.push_back(newStack);
        st=sts.back();//st always refer to the top of stacks.
        callStack.push(ip); 
        ip = target - 1; 
    }

    void do_ret() {
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

    void do_seek() {
        debug_num = st.top();
    }

    void do_print() {
        if (!st.empty()) {
            std::cout <<(int)st.top() << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
    }

    void do_print_fp() {
        if (!st.empty()) {
            uint32_t num = st.top();
            float* floatPtr = (float*)&num;
            std::cout <<*floatPtr << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
    }

    void do_read_fp() {
        uint32_t offset = instructions[++ip];
        float val;
        std::cin >> val; 
        write_mem32(buffer,from_float(val), offset);
    }

    void do_read_int() {
        uint32_t offset = instructions[++ip];
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
    }

    void init_instruction_table() {
        instructionTable[DT_ADD] = &DirectThreadingVM::do_add;
        instructionTable[DT_SUB] = &DirectThreadingVM::do_sub;
        instructionTable[DT_MUL] = &DirectThreadingVM::do_mul;
        instructionTable[DT_DIV] = &DirectThreadingVM::do_div;
        instructionTable[DT_SHL] = &DirectThreadingVM::do_shl;
        instructionTable[DT_SHR] = &DirectThreadingVM::do_shr;
        instructionTable[DT_FP_ADD] = &DirectThreadingVM::do_fp_add;
        instructionTable[DT_FP_SUB] = &DirectThreadingVM::do_fp_sub;
        instructionTable[DT_FP_MUL] = &DirectThreadingVM::do_fp_mul;
        instructionTable[DT_FP_DIV] = &DirectThreadingVM::do_fp_div;
        instructionTable[DT_END] = &DirectThreadingVM::do_end;
        instructionTable[DT_LOD] = &DirectThreadingVM::do_lod;
        instructionTable[DT_STO] = &DirectThreadingVM::do_sto;
        instructionTable[DT_IMMI] = &DirectThreadingVM::do_immi;
        instructionTable[DT_STO_IMMI] = &DirectThreadingVM::do_sto_immi;
        instructionTable[DT_MEMCPY] = &DirectThreadingVM::do_memcpy;
        instructionTable[DT_MEMSET] = &DirectThreadingVM::do_memset;
        instructionTable[DT_INC] = &DirectThreadingVM::do_inc;
        instructionTable[DT_DEC] = &DirectThreadingVM::do_dec;
        instructionTable[DT_JMP] = &DirectThreadingVM::do_jmp;
        instructionTable[DT_JZ] = &DirectThreadingVM::do_jz;
        instructionTable[DT_IF_ELSE] = &DirectThreadingVM::do_if_else;
        instructionTable[DT_JUMP_IF] = &DirectThreadingVM::do_jump_if;
        instructionTable[DT_GT] = &DirectThreadingVM::do_gt;
        instructionTable[DT_LT] = &DirectThreadingVM::do_lt;
        instructionTable[DT_EQ] = &DirectThreadingVM::do_eq;
        instructionTable[DT_GT_EQ] = &DirectThreadingVM::do_gt_eq;
        instructionTable[DT_LT_EQ] = &DirectThreadingVM::do_lt_eq;
        instructionTable[DT_CALL] = &DirectThreadingVM::do_call;
        instructionTable[DT_RET] = &DirectThreadingVM::do_ret;
        instructionTable[DT_SEEK] = &DirectThreadingVM::do_seek;
        instructionTable[DT_PRINT] = &DirectThreadingVM::do_print;
        instructionTable[DT_READ_INT] = &DirectThreadingVM::do_read_int;
        instructionTable[DT_FP_PRINT] = &DirectThreadingVM::do_print_fp;
        instructionTable[DT_FP_READ] = &DirectThreadingVM::do_read_fp;
    }

public:
    uint32_t debug_num;
    DirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) { 
        init_instruction_table();
        debug_num = 0xFFFFFFFF;
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }

    ~DirectThreadingVM() {
        delete[] buffer;
    }

    void run_vm(std::string filename) override{
        try {
            instructions = readFileToUint32Array(filename);
            for (ip = 0; ip < instructions.size(); ip++) {
                (this->*instructionTable[instructions[ip]])();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void run_vm(std::vector<uint32_t>& code) {
        this->instructions = code;
        try {
            for (ip = 0; ip < instructions.size(); ip++) {
                (this->*instructionTable[instructions[ip]])();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};
#endif // TOKENTHREADING_H
                                    