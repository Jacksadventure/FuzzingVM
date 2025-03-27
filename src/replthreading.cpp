#ifndef REPLTHREADING_MODEL
#define REPLTHREADING_MODEL

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
#include <stdlib.h>

class ReplThreadingModel : public Interface {
    // (We still keep these members for memory, stacks, etc.)
    uint32_t ip; 
    std::vector<std::stack<uint32_t>> sts; 
    std::stack<uint32_t> st;
    std::vector<uint32_t> instructions; 
    char* buffer; 
    std::stack<uint32_t> callStack; 

public:
    uint32_t debug_num;
    ReplThreadingModel() : ip(0), buffer(new char[4 * 1024 * 1024]), debug_num(0xFFFFFFFF) {
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }
    ~ReplThreadingModel() {
        delete[] buffer;
    }

    // These helper functions remain available.
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

    // New run_vm() using computed goto (direct threading) as the dispatch method.
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
        // Convert the vector into a pointer for direct threaded code.
        const uint32_t* ip_ptr = instructions.data();

#define NEXT switch(*ip_ptr++) { \
    case DT_ADD:        goto add; \
    case DT_SUB:        goto sub; \
    case DT_MUL:        goto mul; \
    case DT_DIV:        goto div; \
    case DT_SHL:        goto shl; \
    case DT_SHR:        goto shr; \
    case DT_FP_ADD:     goto fp_add; \
    case DT_FP_SUB:     goto fp_sub; \
    case DT_FP_MUL:     goto fp_mul; \
    case DT_FP_DIV:     goto fp_div; \
    case DT_END:        goto end; \
    case DT_LOD:        goto lod; \
    case DT_STO:        goto sto; \
    case DT_IMMI:       goto immi; \
    case DT_STO_IMMI:   goto sto_immi; \
    case DT_MEMCPY:     goto memcpy_inst; \
    case DT_MEMSET:     goto memset_inst; \
    case DT_INC:        goto inc; \
    case DT_DEC:        goto dec; \
    case DT_JMP:        goto jmp; \
    case DT_JZ:         goto jz; \
    case DT_IF_ELSE:    goto if_else; \
    case DT_JUMP_IF:    goto jump_if; \
    case DT_GT:         goto gt; \
    case DT_LT:         goto lt; \
    case DT_EQ:         goto eq; \
    case DT_GT_EQ:      goto gt_eq; \
    case DT_LT_EQ:      goto lt_eq; \
    case DT_CALL:       goto call; \
    case DT_RET:        goto ret; \
    case DT_SEEK:       goto seek; \
    case DT_PRINT:      goto print; \
    case DT_READ_INT:   goto read_int; \
    case DT_FP_PRINT:   goto fp_print; \
    case DT_FP_READ:    goto fp_read; \
    case DT_Tik:        goto tik_inst; \
    default: std::cerr << "Unknown instruction code: " << *(ip_ptr-1) << std::endl; return; \
}

        NEXT;

    // Each label implements the operation then jumps back using NEXT.
    add:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(a + b);
        }
        NEXT;

    sub:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b - a);
        }
        NEXT;

    mul:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(a * b);
        }
        NEXT;

    div:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            if (a == 0) {
                std::cerr << "Error: Divided by zero error" << std::endl;
                return;
            }
            st.push(b / a);
        }
        NEXT;

    shl:
        {
            uint32_t shift = st.top(); st.pop();
            uint32_t value = st.top(); st.pop();
            st.push(value << shift);
        }
        NEXT;

    shr:
        {
            uint32_t shift = st.top(); st.pop();
            uint32_t value = st.top(); st.pop();
            st.push(value >> shift);
        }
        NEXT;

    fp_add:
        {
            float a = to_float(st.top()); st.pop();
            float b = to_float(st.top()); st.pop();
            st.push(from_float(a + b));
        }
        NEXT;

    fp_sub:
        {
            float a = to_float(st.top()); st.pop();
            float b = to_float(st.top()); st.pop();
            st.push(from_float(b - a));
        }
        NEXT;

    fp_mul:
        {
            float a = to_float(st.top()); st.pop();
            float b = to_float(st.top()); st.pop();
            st.push(from_float(a * b));
        }
        NEXT;

    fp_div:
        {
            float a = to_float(st.top()); st.pop();
            float b = to_float(st.top()); st.pop();
            if (a == 0.0f) {
                std::cerr << "Error: Division by zero error" << std::endl;
                return;
            }
            st.push(from_float(b / a));
        }
        NEXT;

    end:
        {
            st = std::stack<uint32_t>();
            instructions = std::vector<uint32_t>();
            return;
        }

    lod:
        {
            uint32_t offset = *ip_ptr++;
            uint32_t a = read_mem32(buffer, offset);
            st.push(a);
        }
        NEXT;

    sto:
        {
            uint32_t offset = *ip_ptr++;
            uint32_t a = st.top(); st.pop();
            write_mem32(buffer, a, offset);
        }
        NEXT;

    immi:
        {
            uint32_t a = *ip_ptr++;
            st.push(a);
        }
        NEXT;

    sto_immi:
        {
            uint32_t offset = *ip_ptr++;
            uint32_t number = *ip_ptr++;
            write_mem32(buffer, number, offset);
        }
        NEXT;

    memcpy_inst:
        {
            uint32_t dest = *ip_ptr++;
            uint32_t src = *ip_ptr++;
            uint32_t len  = *ip_ptr++;
            memcpy(buffer + dest, buffer + src, len);
        }
        NEXT;

    memset_inst:
        {
            uint32_t dest = *ip_ptr++;
            uint32_t val  = *ip_ptr++;
            uint32_t len  = *ip_ptr++;
            memset(buffer + dest, val, len);
        }
        NEXT;

    inc:
        {
            uint32_t a = st.top(); st.pop();
            st.push(++a);
        }
        NEXT;

    dec:
        {
            uint32_t a = st.top(); st.pop();
            st.push(--a);
        }
        NEXT;

    jmp:
        {
            uint32_t target = *ip_ptr++;
            ip_ptr = instructions.data() + target;
        }
        NEXT;

    jz:
        {
            uint32_t target = *ip_ptr++;
            uint32_t topVal = st.top(); st.pop();
            if (topVal == 0) {
                ip_ptr = instructions.data() + target;
            }
        }
        NEXT;

    jump_if:
        {
            uint32_t condition = st.top(); st.pop();
            uint32_t target = *ip_ptr++;
            if (condition) {
                ip_ptr = instructions.data() + target;
            }
        }
        NEXT;

    if_else:
        {
            uint32_t condition = st.top(); st.pop();
            uint32_t trueBranch = *ip_ptr++;
            uint32_t falseBranch = *ip_ptr++;
            ip_ptr = instructions.data() + (condition ? trueBranch : falseBranch);
        }
        NEXT;

    gt:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b > a ? 1 : 0);
        }
        NEXT;

    lt:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b < a ? 1 : 0);
        }
        NEXT;

    eq:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b == a ? 1 : 0);
        }
        NEXT;

    gt_eq:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b >= a ? 1 : 0);
        }
        NEXT;

    lt_eq:
        {
            uint32_t a = st.top(); st.pop();
            uint32_t b = st.top(); st.pop();
            st.push(b <= a ? 1 : 0);
        }
        NEXT;

    call:
        {
            uint32_t target = *ip_ptr++;
            uint32_t num_params = *ip_ptr++;
            std::stack<uint32_t> newStack;
            for (uint32_t i = 0; i < num_params; ++i) {
                newStack.push(st.top());
                st.pop();
            }
            sts.push_back(newStack);
            st = sts.back();
            // Save current instruction pointer (as an index)
            callStack.push(ip_ptr - instructions.data());
            ip_ptr = instructions.data() + target;
        }
        NEXT;

    ret:
        {
            if (callStack.empty()) {
                std::cerr << "Error: Call stack underflow" << std::endl;
                return;
            }
            uint32_t return_value = st.top();
            st.pop();
            uint32_t ret_index = callStack.top();
            callStack.pop();
            sts.pop_back();
            if (!sts.empty()) {
                st = sts.back();
            }
            st.push(return_value);
            ip_ptr = instructions.data() + ret_index;
        }
        NEXT;

    seek:
        {
            debug_num = st.top();
        }
        NEXT;

    print:
        {
            if (!st.empty()) {
                std::cout << static_cast<int>(st.top()) << std::endl;
            } else {
                std::cerr << "Stack is empty." << std::endl;
            }
        }
        NEXT;

    read_int:
        {
            uint32_t offset = *ip_ptr++;
            int val;
            std::cin >> val;
            write_mem32(buffer, val, offset);
        }
        NEXT;

    fp_print:
        {
            if (!st.empty()) {
                uint32_t num = st.top();
                float* floatPtr = reinterpret_cast<float*>(&num);
                std::cout << *floatPtr << std::endl;
            } else {
                std::cerr << "Stack is empty." << std::endl;
            }
        }
        NEXT;

    fp_read:
        {
            uint32_t offset = *ip_ptr++;
            float val;
            std::cin >> val;
            write_mem32(buffer, from_float(val), offset);
        }
        NEXT;

    tik_inst:
        {
            std::cout << "tik" << std::endl;
        }
        NEXT;

#undef NEXT
    }
};

#endif // REPLTHREADING_MODEL