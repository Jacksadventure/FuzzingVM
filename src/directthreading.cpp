#ifndef DIRECTTHREADING_H
#define DIRECTTHREADING_H
#include <vector>
#include <stack>
#include <iostream>
#include <unistd.h>   
#include <fcntl.h>    
#include <sys/types.h> 
#include <sys/stat.h>  
#ifdef _WIN32
#include <windows.h> // Windows-specific headers for file operations
#endif
#include "symbol.hpp"
class DirectThreadingVM {
private:
    uint32_t ip; // Instruction pointer
    std::stack<uint32_t> st; // Stack for operations
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

    void handle_syscall(uint32_t syscall_num) {
    switch (syscall_num) {
        case SYS_WRITE: {
            int fd = st.top(); st.pop();
            uint32_t bufPtr = st.top(); st.pop();
            size_t count = st.top(); st.pop();
            ssize_t written = write(fd, buffer + bufPtr, count);
            st.push(written);
            break;
        }
        case SYS_READ: {
            int fd = st.top(); st.pop();
            uint32_t bufPtr = st.top(); st.pop();
            size_t count = st.top(); st.pop();
            ssize_t readBytes = read(fd, buffer + bufPtr, count);
            st.push(readBytes);
            break;
        }
        case SYS_OPEN: {
            uint32_t filenamePtr = st.top(); st.pop();
            int flags = st.top(); st.pop();
            int mode = st.top(); st.pop(); 
            int fd = open(reinterpret_cast<char*>(buffer + filenamePtr), flags, mode);
            st.push(fd);
            break;
        }
        case SYS_CLOSE: {
            int fd = st.top(); st.pop();
            int result = close(fd);
            st.push(result);
            break;
        }
        case SYS_LSEEK: {
            int fd = st.top(); st.pop();
            off_t offset = st.top(); st.pop();
            int whence = st.top(); st.pop();
            off_t newPos = lseek(fd, offset, whence);
            st.push(newPos);
            break;
        }
    }
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
    //untested_unsafe
    void do_call() {
        uint32_t target = instructions[++ip]; // Function address
        uint32_t num_params = instructions[++ip]; // Number of parameters
        callStack.push(ip); // Save the return address
        callStack.push(num_params); // Save the number of parameters
        ip = target - 1; // Jump to the function
    }
    //untested_unsafe
    void do_ret() {
        if (callStack.empty()) {
            std::cerr << "Error: Call stack underflow" << std::endl;
            return;
        }
        ip = callStack.top(); callStack.pop(); // Return address
        uint32_t num_params = callStack.top(); callStack.pop(); // Number of parameters
        // Clean up parameters from the stack
        while (num_params--) {
            st.pop();
        }
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

    void do_fp_print() {
        if (!st.empty()) {
            std::cout <<(float)st.top() << std::endl;
        } else {
            std::cerr << "Stack is empty." << std::endl;
        }
    }

    void do_read_fp() {
        uint32_t offset = instructions[++ip];
        float val;
        std::cin >> val; 
        write_mem32(buffer, val, offset);
    }

    void do_read_int() {
        uint32_t offset = instructions[++ip];
        int val;
        std::cin >> val;
        write_mem32(buffer, val, offset);
    }

    void do_syscall() {
        uint32_t syscall_num = instructions[++ip];
        handle_syscall(syscall_num);
    }

    void init_instruction_table() {
        instructionTable[DT_ADD] = &DirectThreadingVM::do_add;
        instructionTable[DT_SUB] = &DirectThreadingVM::do_sub;
        instructionTable[DT_MUL] = &DirectThreadingVM::do_mul;
        instructionTable[DT_DIV] = &DirectThreadingVM::do_div;
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
        instructionTable[DT_FP_PRINT] = &DirectThreadingVM::do_fp_print;
        instructionTable[DT_FP_READ] = &DirectThreadingVM::do_read_fp;
        instructionTable[DT_SYSCALL] = &DirectThreadingVM::do_syscall;
    }

public:
    uint32_t debug_num;
    DirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) { // Initialize a 4MB buffer
        init_instruction_table();
        debug_num = 0xFFFFFFFF;
    }

    ~DirectThreadingVM() {
        delete[] buffer;
    }

    void run_vm(const std::vector<uint32_t>& ins) {
        instructions = ins;
        for (ip = 0; ip < instructions.size(); ip++) {
            (this->*instructionTable[instructions[ip]])();
        }
    }
};
#endif // DIRECTTHREADING_H


