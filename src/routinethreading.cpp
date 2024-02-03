#ifndef ROUTINETHREADING_H
#define ROUTINETHREADING_H

#include <vector>
#include <stack>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <map>
#include <unordered_set>   
#include "readfile.hpp"
#include "interface.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include "symbol.hpp"

struct pair_hash {
    size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
        auto hash1 = std::hash<uint32_t>{}(p.first);
        auto hash2 = std::hash<uint32_t>{}(p.second);
        return hash1 ^ hash2; // 组合两个哈希值
    }
};

class RoutineThreadingVM :public Interface{
private:
    uint32_t ip; // Instruction pointer
    std::vector<std::stack<uint32_t> > sts; // Stacks for operations
    std::stack<uint32_t> st;
    std::vector<std::vector<uint32_t> > instructions; // Instruction set
    char* buffer; // Memory buffer
    std::stack<uint32_t> callStack; // Call stack for function calls
    float to_float(uint32_t val) {
        return *reinterpret_cast<float*>(&val);
    }

    uint32_t from_float(float val) {
        return *reinterpret_cast<uint32_t*>(&val);
    }

    void write_memory(char* buffer, uint32_t* src, uint32_t offset, uint32_t size) {
        memcpy(buffer + offset, src, size);
    }

    void write_mem32(char* buffer, uint32_t val, uint32_t offset) {
        uint32_t buf[1] = { val };
        write_memory(buffer, buf, offset, 4);
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
        instructions = std::vector<std::vector<uint32_t> >();
        ip = 0;
    }

    void do_lod(uint32_t offset) {
        uint32_t a = read_mem32(buffer,offset);
        st.push(a);
    }

    void do_sto(uint32_t offset) {
        uint32_t a = st.top(); st.pop();
        write_mem32(buffer,a,offset);
    }

    void do_immi(uint32_t a) {
        st.push(a);
    }

    void do_memcpy(uint32_t dest,uint32_t src,uint32_t len) {

        memcpy(buffer + dest, buffer + src, len);
    }

    void do_memset(uint32_t dest,uint32_t val,uint32_t len) {
        memset(buffer + dest, val, len);
    }
    void do_sto_immi(uint32_t offset,uint32_t number) {
        write_mem32(buffer,number,offset);
    }

    void do_jmp(uint32_t target) {
        ip = target - 1;
    }

    void do_jz(uint32_t target) {
        if (st.top() == 0) {
            ip = target - 1;
        }
        st.pop();
    }

    void do_jump_if(uint32_t target) {
        uint32_t condition = st.top(); st.pop();
        if (condition) {
            ip = target - 1;
        }
    }

    void do_if_else(uint32_t trueBranch, uint32_t falseBranch) {
        uint32_t condition = st.top(); st.pop();
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

    void do_call(uint32_t target, uint32_t num_params) {
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

    void do_read_fp(uint32_t offset) {
        float val;
        std::cin >> val; 
        write_mem32(buffer,from_float(val), offset);
    }

    void do_read_int(uint32_t offset) {
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
    }

public:
    uint32_t debug_num;
    RoutineThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) {
        debug_num = 0xFFFFFFFF;
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }

    ~RoutineThreadingVM() {
        delete[] buffer;
    }
    
    void run_vm(std::string filename) override {
        std::vector<uint32_t> code = readFileToUint32Array(filename);
        std::map<int, int> addressMap;  
        std::vector<std::vector<uint32_t>> instructions;
        uint32_t pointer = 0;  
        uint32_t instructionIndex = 0;  
        while(pointer < code.size()){
            uint32_t instructionStart = pointer;
            std::vector<uint32_t> instruction;  
            instruction.push_back(code[pointer++]);
            switch (instruction[0]) {
                case DT_LOD:
                case DT_STO:
                case DT_IMMI:
                case DT_READ_INT:
                case DT_FP_READ:
                case DT_JMP:
                case DT_JZ:
                case DT_JUMP_IF:
                    instruction.push_back(code[pointer++]);
                    break;
                case DT_MEMCPY:
                case DT_MEMSET:
                case DT_STO_IMMI:
                case DT_IF_ELSE:
                case DT_CALL:
                    instruction.push_back(code[pointer++]);
                    instruction.push_back(code[pointer++]);
                    break;
            }

            instructions.push_back(instruction);
            addressMap[instructionStart] = instructionIndex++;
        }
        for (auto &instruction : instructions) {
            switch (instruction[0]) {
                case DT_JMP:
                case DT_JZ:
                case DT_JUMP_IF:
                case DT_CALL:
                    if (instruction.size() > 1) {
                        instruction[1] = addressMap[instruction[1]];
                    }
                    break;
                case DT_IF_ELSE:
                    if (instruction.size() > 2) {
                        instruction[1] = addressMap[instruction[1]];
                        instruction[2] = addressMap[instruction[2]];
                    }
                    break;
            }
        }
        run_vm(instructions);
    }

    void run_vm(const std::vector<std::vector<uint32_t>>& ins) {
        instructions = ins;
        for (ip = 0; ip < instructions.size(); ip++) {
            switch (instructions[ip][0]) {
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
                    do_lod(instructions[ip][1]);
                    break;
                case DT_STO:
                    do_sto(instructions[ip][1]);
                    break;
                case DT_IMMI:
                    do_immi(instructions[ip][1]);
                    break;
                case DT_INC:
                    do_inc();
                    break;
                case DT_DEC:
                    do_dec();
                    break;
                case DT_STO_IMMI:
                    do_sto_immi(instructions[ip][1],instructions[ip][2]);
                    break;
                case DT_MEMCPY:
                    do_memcpy(instructions[ip][1],instructions[ip][2],instructions[ip][3]);
                    break;
                case DT_MEMSET:
                    do_memset(instructions[ip][1],instructions[ip][2],instructions[ip][3]);
                    break;
                case DT_JMP:
                    do_jmp(instructions[ip][1]);
                    break;
                case DT_JZ:
                    do_jz(instructions[ip][1]);
                    break;
                case DT_JUMP_IF:
                    do_jump_if(instructions[ip][1]);
                    break;
                case DT_IF_ELSE:
                    do_if_else(instructions[ip][1], instructions[ip][2]);
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
                    do_call(instructions[ip][1], instructions[ip][2]);
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
                    do_read_int(instructions[ip][1]);
                    break;
                case DT_FP_PRINT:
                    do_print_fp();
                    break;
                case DT_FP_READ:
                    do_read_fp(instructions[ip][1]);
                    break;
                default:
                    std::cerr << "Error: Unknown subroutine "<< std::endl;
            }
        }
    }

    static std::vector<uint32_t> convertToVMFormat(const std::string& input) {
        std::vector<uint32_t> output;
        for (char c : input) {
            output.push_back(static_cast<uint32_t>(c));
        }
        output.push_back(static_cast<uint32_t>('\0'));
        return output;
    }

    char* getBuffer() {
        return buffer;
    }
};

#endif 
