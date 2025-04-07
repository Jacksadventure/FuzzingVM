#ifndef SWTHREADING
#define SWTHREADING

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
#include <windows.h>
#endif

class SwThreadingVM : public Interface {
    uint32_t ip; 
    std::vector<std::stack<uint32_t>> sts; 
    std::stack<uint32_t> st;
    std::vector<uint32_t> instructions; 
    char* buffer; 
    std::stack<uint32_t> callStack; 
    uint32_t seed = 2463534242UL; // Seed for random number generation
    inline uint32_t rd() {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed;
    }
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
    inline void do_mod() {
        uint32_t a = st.top(); st.pop();
        uint32_t b = st.top(); st.pop();
        if (a == 0) { 
            std::cerr << "Error: Modulo by zero error" << std::endl;
            return;
        }
        st.push(b % a);
    }
    inline void do_dup() {
        if (st.empty()) {
            std::cerr << "Error: Stack is empty" << std::endl;
            return;
        }
        uint32_t a = st.top();
        st.push(a);
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
        if (a == 0.0f) { 
            std::cerr << "Error: Division by zero error" << std::endl;
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
        uint32_t offset = instructions[ip++];
        uint32_t a = read_mem32(buffer, offset);
        st.push(a);
    }
    inline void do_sto() {
        uint32_t offset = instructions[ip++];
        uint32_t a = st.top(); st.pop();
        write_mem32(buffer, a, offset);
    }
    inline void do_immi() {
        uint32_t a = instructions[ip++];
        st.push(a);
    }
    inline void do_memcpy() {
        uint32_t dest = instructions[ip++];
        uint32_t src = instructions[ip++];
        uint32_t len  = instructions[ip++];
        memcpy(buffer + dest, buffer + src, len);
    }
    inline void do_memset() {
        uint32_t dest = instructions[ip++];
        uint32_t val  = instructions[ip++];
        uint32_t len  = instructions[ip++];
        memset(buffer + dest, val, len);
    }
    inline void do_sto_immi() {
        uint32_t offset = instructions[ip++];
        uint32_t number = instructions[ip++];
        write_mem32(buffer, number, offset);
    }

    // 将 do_jmp 修改为使用相对偏移量
    inline void do_jmp() {
        int32_t offset = static_cast<int32_t>(instructions[ip++]);
        ip = ip + offset;
    }

    // 修改 do_jz，使其跳转目标为当前 ip 加上相对偏移量
    inline void do_jz() {
        int32_t offset = static_cast<int32_t>(instructions[ip++]);
        uint32_t topVal = st.top(); st.pop();
        if (topVal == 0) {
            ip = ip + offset;
        }
    }

    // 修改 do_jump_if，同样使用相对偏移量
    inline void do_jump_if() {
        uint32_t condition = st.top(); st.pop();
        int32_t offset = static_cast<int32_t>(instructions[ip++]);
        if (condition) {
            ip = ip + offset;
        }
    }

    // 修改 do_if_else，分别对 true 和 false 分支使用相对偏移量
    inline void do_if_else() {
        uint32_t condition = st.top(); st.pop();
        int32_t trueOffset = static_cast<int32_t>(instructions[ip++]);
        int32_t falseOffset = static_cast<int32_t>(instructions[ip++]);
        ip = condition ? (ip + trueOffset) : (ip + falseOffset);
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
        uint32_t target = instructions[ip++];
        uint32_t num_params = instructions[ip++];
        std::stack<uint32_t> newStack;
        for (uint32_t i = 0; i < num_params; ++i) {
            newStack.push(st.top());
            st.pop();
        }
        sts.push_back(newStack);
        st = sts.back(); 
        callStack.push(ip+num_params);
        ip = target;
    }
    inline void do_ret() {
        if (callStack.size() == 0) {
            exit(0);
        }
        st.pop();
        ip = callStack.top(); callStack.pop();
        sts.pop_back();
        if (!sts.empty()) {
            st = sts.back();
        }
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
        uint32_t offset = instructions[ip++];
        float val;
        std::cin >> val;
        write_mem32(buffer, from_float(val), offset);
    }
    inline void do_read_int() {
        uint32_t offset = instructions[ip++];
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
    }
    inline void tik() {
        std::cout << "tik" << std::endl;
    }
    inline void do_rnd() {
        if (!st.empty()) {
            uint32_t a = st.top(); st.pop();
            st.push(rd() % a);
        }
    }
public:
    uint32_t debug_num;
    SwThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]), debug_num(0xFFFFFFFF) {
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }
    ~SwThreadingVM() {
        delete[] buffer;
    }


    void run_vm(std::string filename, bool benchmarkMode) {
        try {
            instructions = readFileToUint32Array(filename);
            if (benchmarkMode) {
                std::cout << "Preprocessing completed, starting benchmark..." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
        ip = 0;
        while (ip < instructions.size()) {
            uint32_t opcode = instructions[ip];
            ip++;
            switch(opcode) {
                case DT_ADD:
                    do_add();
                    break;
                case DT_SUB:
                    do_sub();
                    break;
                case DT_MUL:
                    do_mul();
                    break;
                case DT_DIV:
                    do_div();
                    break;
                case DT_MOD:
                    do_mod();
                    break;
                case DT_SHL:
                    do_shl();
                    break;
                case DT_SHR:
                    do_shr();
                    break;
                case DT_FP_ADD:
                    do_fp_add();
                    break;
                case DT_FP_SUB:
                    do_fp_sub();
                    break;
                case DT_FP_MUL:
                    do_fp_mul();
                    break;
                case DT_FP_DIV:
                    do_fp_div();
                    break;
                case DT_END:
                    do_end();
                    break;
                case DT_LOD:
                    do_lod();
                    break;
                case DT_STO:
                    do_sto();
                    break;
                case DT_IMMI:
                    do_immi();
                    break;
                case DT_STO_IMMI:
                    do_sto_immi();
                    break;
                case DT_MEMCPY:
                    do_memcpy();
                    break;
                case DT_MEMSET:
                    do_memset();
                    break;
                case DT_INC:
                    do_inc();
                    break;
                case DT_DEC:
                    do_dec();
                    break;
                case DT_JMP:
                    do_jmp();
                    break;
                case DT_JZ:
                    do_jz();
                    break;
                case DT_IF_ELSE:
                    do_if_else();
                    break;
                case DT_JUMP_IF:
                    do_jump_if();
                    break;
                case DT_GT:
                    do_gt();
                    break;
                case DT_LT:
                    do_lt();
                    break;
                case DT_EQ:
                    do_eq();
                    break;
                case DT_GT_EQ:
                    do_gt_eq();
                    break;
                case DT_LT_EQ:
                    do_lt_eq();
                    break;
                case DT_CALL:
                    do_call();
                    break;
                case DT_RET:
                    do_ret();
                    break;
                case DT_SEEK:
                    do_seek();
                    break;
                case DT_PRINT:
                    do_print();
                    break;
                case DT_READ_INT:
                    do_read_int();
                    break;
                case DT_FP_PRINT:
                    do_print_fp();
                    break;
                case DT_FP_READ:
                    do_read_fp();
                    break;
                case DT_Tik:
                    tik();
                    break;
                case DT_RND:
                    do_rnd();
                    break;
                case DT_DUP:
                    do_dup();
                    break;
                default:
                    std::cerr << "Unknown instruction code: " << opcode << std::endl;
                    return;
            }
        }
    }
};

#endif // SWTHREADING