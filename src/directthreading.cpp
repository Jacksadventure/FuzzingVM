#ifndef DIRECTTHREADINGVM_H
#define DIRECTTHREADINGVM_H

#include <vector>
#include <stack>
#include <iostream>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include "readfile.hpp"
#include "interface.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include "symbol.hpp"

class DirectThreadingVM : public Interface {
private:
    uint32_t ip; // Instruction pointer (not used in the generated C file)
    std::vector<std::stack<uint32_t>> sts; // Collection of operand stacks
    std::stack<uint32_t> st;             // Current operand stack (st = sts.back())
    std::vector<uint32_t> instructions;  // Parsed instruction stream (including operands)
    char* buffer;                        // Memory buffer
    std::stack<uint32_t> callStack;      // Call stack

    // Returns the number of immediate operands expected for an opcode.
    int operandCount(uint32_t opcode) {
        switch (opcode) {
            case DT_ADD:
            case DT_SUB:
            case DT_MUL:
            case DT_DIV:
            case DT_MOD:
            case DT_SHL:
            case DT_SHR:
            case DT_FP_ADD:
            case DT_FP_SUB:
            case DT_FP_MUL:
            case DT_FP_DIV:
            case DT_END:
            case DT_INC:
            case DT_DEC:
            case DT_GT:
            case DT_LT:
            case DT_EQ:
            case DT_GT_EQ:
            case DT_LT_EQ:
            case DT_PRINT:
            case DT_FP_PRINT:
            case DT_Tik:
            case DT_RET:
            case DT_RND:
            case DT_DUP:
                return 0;
            case DT_LOD:
            case DT_STO:
            case DT_IMMI:
            case DT_READ_INT:
            case DT_FP_READ:
            case DT_JMP:
            case DT_JZ:
            case DT_JUMP_IF:
            case DT_SEEK:
                return 1;
            case DT_STO_IMMI:
            case DT_IF_ELSE:
            case DT_CALL:  // Modified: DT_CALL now takes 2 parameters (target, num_params)
                return 2;
            case DT_MEMCPY:
            case DT_MEMSET:
                return 3;
            default:
                return 0;
        }
    }

public:
    DirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) {
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }
    ~DirectThreadingVM() {
        delete[] buffer;
    }
    
    // run_vm:
    // Reads the instruction stream from file and generates a pure C source file that implements
    // the virtual machine using direct threading. The generated code separates instructions and
    // immediate values, and uses label pointers for correct computed goto implementation.
    void run_vm(std::string filename, bool benchmarkMode) {
        try {
            instructions = readFileToUint32Array(filename);
        } catch (const std::exception &e) {
            std::cerr << "Error reading file: " << e.what() << std::endl;
            return;
        }
        
        // First, analyze the instruction stream to separate opcodes and operands
        std::vector<uint32_t> opcodes;         // Only opcodes
        std::vector<uint32_t> immediateValues; // Only immediate values
        std::vector<int> opToImmIndices;       // Maps each opcode to its first immediate index
        
        // Array to map instruction index to label index
        std::vector<int> instToLabelIndex;
        // 新增：记录原始操作码在指令流中的位置
        std::vector<uint32_t> opcode_orig_indices;
        
        size_t raw = 0;
        size_t immediateIndex = 0;
        while (raw < instructions.size()) {
            uint32_t opcode = instructions[raw++];
            opcodes.push_back(opcode);
            instToLabelIndex.push_back(opcodes.size() - 1);
            
            opToImmIndices.push_back(immediateIndex);
            // 记录当前操作码在原始数组中的位置（raw-1 为opcode所在位置）
            opcode_orig_indices.push_back(raw - 1);
            
            int nOperands = operandCount(opcode);
            for (int i = 0; i < nOperands && raw < instructions.size(); i++) {
                immediateValues.push_back(instructions[raw++]);
                immediateIndex++;
            }
        }
        
        // Generate the output file.
        std::string output_filename = filename + "_compiled_dt" + ".c";
        std::ofstream out(output_filename);
        if (!out) {
            std::cerr << "Unable to open file " << output_filename << " for writing." << std::endl;
            return;
        }
        
        // Write standard headers and macro definitions.
        out << "#include <stdio.h>\n#include <stdlib.h>\n#include <stdint.h>\n#include <string.h>\n";
        out << "#include <time.h>\n\n"; // For random number generation
        
        out << "#define STACK_SIZE 1024\n#define MAX_STACKS 64\n#define BUFFER_SIZE (4 * 1024 * 1024)\n\n";
        
        // Global variables - modified to support multiple stacks
        out << "// Main stack array\n";
        out << "uint32_t stacks[MAX_STACKS][STACK_SIZE];\n";
        out << "int stack_tops[MAX_STACKS] = {-1};\n";
        out << "int current_stack = 0; // Index of current stack\n\n";
        
        out << "// Memory buffer\n";
        out << "char buffer[BUFFER_SIZE];\n";
        
        out << "// Call stack\n";
        out << "uint32_t callStack[STACK_SIZE];\n";
        out << "int call_top = -1; // Call stack top\n";
        
        out << "// Stack context information for function calls\n";
        out << "struct StackContext {\n";
        out << "    int stack_index;\n";
        out << "    int return_ip;\n";
        out << "};\n";
        out << "struct StackContext stack_contexts[STACK_SIZE];\n\n";
        
        out << "uint32_t debug_num = 0; // For DT_SEEK\n\n";
        out << "int32_t ip = -1; // Instruction pointer\n\n";

        // Helper conversion functions.
        out << "float to_float(uint32_t val) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.i = val; return u.f;\n}\n";
        out << "uint32_t from_float(float f) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.f = f; return u.i;\n}\n\n";
        
        // Stack operation macros
        out << "// Stack operation macros\n";
        out << "#define PUSH(val) stacks[current_stack][++stack_tops[current_stack]] = (val)\n";
        out << "#define POP() stacks[current_stack][stack_tops[current_stack]--]\n";
        out << "#define TOP() stacks[current_stack][stack_tops[current_stack]]\n";
        out << "#define STACK_TOP stack_tops[current_stack]\n\n";
        
        // Define the NEXT macro (computed goto).
        out << "#define NEXT goto *labels[++ip]\n\n";
        out << "uint32_t seed = 2463534242UL; // Seed for random number generation\n";
        out << "static inline uint32_t rd() {\n";
        out << "    seed ^= seed << 13;\n";
        out << "    seed ^= seed >> 17;\n";
        out << "    seed ^= seed << 5;\n";
        out << "    return seed;\n}\n\n";
        // Embedded instruction implementation functions - modified for the new stack architecture
        out << "static inline void do_add() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH(a + b);\n}\n\n";
        
        out << "static inline void do_sub() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH(b - a);\n}\n\n";
        
        out << "static inline void do_mul() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH(a * b);\n}\n\n";
        
        out << "static inline void do_div() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    if(a == 0) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n"
               "    PUSH(b / a);\n}\n\n";
        
        out << "static inline void do_mod() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    if(a == 0) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n"
               "    PUSH(b % a);\n}\n\n";

        out << "static inline void do_shl() {\n"
               "    uint32_t shift = POP();\n"
               "    uint32_t value = POP();\n"
               "    PUSH(value << shift);\n}\n\n";
        
        out << "static inline void do_shr() {\n"
               "    uint32_t shift = POP();\n"
               "    uint32_t value = POP();\n"
               "    PUSH(value >> shift);\n}\n\n";
        
        out << "static inline void do_fp_add() {\n"
               "    float a = to_float(POP());\n"
               "    float b = to_float(POP());\n"
               "    PUSH(from_float(a + b));\n}\n\n";
        
        out << "static inline void do_fp_sub() {\n"
               "    float a = to_float(POP());\n"
               "    float b = to_float(POP());\n"
               "    PUSH(from_float(b - a));\n}\n\n";
        
        out << "static inline void do_fp_mul() {\n"
               "    float a = to_float(POP());\n"
               "    float b = to_float(POP());\n"
               "    PUSH(from_float(a * b));\n}\n\n";
        
        out << "static inline void do_fp_div() {\n"
               "    float a = to_float(POP());\n"
               "    float b = to_float(POP());\n"
               "    if(a == 0.0f) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n"
               "    PUSH(from_float(b / a));\n}\n\n";
        
        out << "static inline void do_end() {\n"
               "    // Reset all stacks\n"
               "    for (int i = 0; i < MAX_STACKS; i++) {\n"
               "        stack_tops[i] = -1;\n"
               "    }\n"
               "    current_stack = 0;\n"
               "    memset(buffer, 0, BUFFER_SIZE);\n"
               "}\n\n";
        
        out << "static inline void do_lod(uint32_t offset) {\n"
               "    uint32_t value;\n"
               "    memcpy(&value, buffer + offset, sizeof(uint32_t));\n"
               "    PUSH(value);\n}\n\n";
        
        out << "static inline void do_sto(uint32_t offset) {\n"
               "    uint32_t value = POP();\n"
               "    memcpy(buffer + offset, &value, sizeof(uint32_t));\n}\n\n";
        
        out << "static inline void do_immi(uint32_t value) {\n"
               "    PUSH(value);\n}\n\n";
        
        out << "static inline void do_inc() {\n"
               "    uint32_t value = POP();\n"
               "    PUSH(value + 1);\n}\n\n";
        
        out << "static inline void do_dec() {\n"
               "    uint32_t value = POP();\n"
               "    PUSH(value - 1);\n}\n\n";
        
        out << "static inline void do_sto_immi(uint32_t offset, uint32_t number) {\n"
               "    memcpy(buffer + offset, &number, sizeof(uint32_t));\n}\n\n";
        
        out << "static inline void do_memcpy(uint32_t dest, uint32_t src, uint32_t len) {\n"
               "    memcpy(buffer + dest, buffer + src, len);\n}\n\n";
        
        out << "static inline void do_memset(uint32_t dest, uint32_t val, uint32_t len) {\n"
               "    memset(buffer + dest, val, len);\n}\n\n";
        
        out << "static inline void do_gt() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH((b > a) ? 1 : 0);\n}\n\n";
        
        out << "static inline void do_lt() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH((b < a) ? 1 : 0);\n}\n\n";
        
        out << "static inline void do_eq() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH((b == a) ? 1 : 0);\n}\n\n";
        
        out << "static inline void do_gt_eq() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH((b >= a) ? 1 : 0);\n}\n\n";
        
        out << "static inline void do_lt_eq() {\n"
               "    uint32_t a = POP();\n"
               "    uint32_t b = POP();\n"
               "    PUSH((b <= a) ? 1 : 0);\n}\n\n";
        
        out << "static inline void do_dup() {\n"
               "    if (STACK_TOP >= 0) {\n"
               "        uint32_t value = TOP();\n"
               "        PUSH(value);\n"
               "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n"
               "}\n\n";
        out << "static inline void do_print() {\n"
               "    if (STACK_TOP >= 0) {\n"
               "        printf(\"%u\\n\", TOP());\n"
               "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n"
               "}\n\n";
        
        out << "static inline void do_read_int(uint32_t offset) {\n"
               "    uint32_t val;\n"
               "    scanf(\"%u\", &val);\n"
               "    memcpy(buffer + offset, &val, sizeof(uint32_t));\n"
               "}\n\n";
        
        out << "static inline void do_fp_print() {\n"
               "    if (STACK_TOP >= 0) {\n"
               "        float f = to_float(TOP());\n"
               "        printf(\"%f\\n\", f);\n"
               "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n"
               "}\n\n";
        
        out << "static inline void do_fp_read(uint32_t offset) {\n"
               "    float val;\n"
               "    scanf(\"%f\", &val);\n"
               "    uint32_t ival = from_float(val);\n"
               "    memcpy(buffer + offset, &ival, sizeof(uint32_t));\n"
               "}\n\n";
        
        // Updated do_call to handle parameter passing and multiple stacks
        out << "static inline void do_call(uint32_t target, uint32_t num_params) {\n"
               "    // Save current stack context\n"
               "    stack_contexts[call_top].stack_index = current_stack;\n"
               "    stack_contexts[call_top].return_ip = ip;\n"
               "\n"
               "    // Create a new stack for the function\n"
               "    int new_stack = current_stack + 1;\n"
               "    if (new_stack >= MAX_STACKS) {\n"
               "        fprintf(stderr, \"Error: Stack overflow, too many nested function calls\\n\");\n"
               "        exit(1);\n"
               "    }\n"
               "    \n"
               "    // Transfer parameters from current stack to new stack\n"
               "    // Parameters are in reverse order on the caller's stack\n"
               "    for (uint32_t i = 0; i < num_params; i++) {\n"
               "        if (stack_tops[current_stack] < 0) {\n"
               "            fprintf(stderr, \"Error: Stack underflow during parameter passing\\n\");\n"
               "            exit(1);\n"
               "        }\n"
               "        // Pop from current stack\n"
               "        uint32_t param = stacks[current_stack][stack_tops[current_stack]--];\n"
               "        \n"
               "        // Push onto new stack (in reverse order to maintain original order)\n"
               "        stacks[new_stack][i] = param;\n"
               "    }\n"
               "    \n"
               "    // Set top index for new stack\n"
               "    stack_tops[new_stack] = num_params - 1;\n"
               "    \n"
               "    // Switch to the new stack\n"
               "    current_stack = new_stack;\n"
               "    \n"
               "    // Save the call stack\n"
               "    callStack[++call_top] = ip;\n"
               "}\n\n";
        
        // Updated do_ret to restore previous stack
        out << "static inline int do_ret() {\n"
               "    if (call_top >= 0) {\n"
            //    "        // Get the return value (if any) from current stack\n"
            //    "        uint32_t return_value = 0;\n"
            //    "        if (stack_tops[current_stack] >= 0) {\n"
            //    "            return_value = stacks[current_stack][stack_tops[current_stack]];\n"
            //    "        }\n"
            //    "        \n"
               "        // Restore previous stack\n"
               "        current_stack = stack_contexts[call_top].stack_index;\n"
               "        \n"
            //    "        // Push return value onto restored stack if there was one\n"
            //    "        if (stack_tops[current_stack] < STACK_SIZE - 1) {\n"
            //    "            stacks[current_stack][++stack_tops[current_stack]] = return_value;\n"
            //    "        }\n"
            //    "        \n"
               "        return callStack[call_top--];\n"
               "    } else {\n"
               "        exit(0);\n"
               "    }\n"
               "    return -1;\n"
               "}\n\n";
        
        out << "static inline void do_tik() { printf(\"tik\\n\"); }\n\n";
        
        out << "static inline void do_seek(uint32_t value) {\n"
               "    debug_num = value;\n"
               "}\n\n";
        
        out << "static inline void do_rnd() {\n"
               "    uint32_t max = POP();\n"
               "    if (max == 0) {\n"
               "        PUSH(0);\n"
               "        return;\n"
               "    }\n"
               "    PUSH(rd() % max);\n"
               "}\n\n";
        
        // Generate main function with separate label and immediate arrays
        out << "int main() {\n";
        out << "    // Initialize random number generator\n";
        out << "    srand((unsigned int)time(NULL));\n\n";
        
        // Generate the labels array for each opcode
        out << "    // Label pointer array for computed goto\n";
        out << "    void* labels[] = {\n";
        for (size_t i = 0; i < opcodes.size(); i++) {
            out << "        &&L" << i;
            if (i < opcodes.size() - 1) 
                out << ",";
            out << " // Opcode: " << opcodes[i] << "\n";
        }
        out << "    };\n\n";
        
        // Generate the immediate values array
        if (!immediateValues.empty()) {
            out << "    // Immediate values array\n";
            out << "    uint32_t immediates[] = {\n";
            for (size_t i = 0; i < immediateValues.size(); i++) {
                out << "        " << immediateValues[i];
                if (i < immediateValues.size() - 1) 
                    out << ",";
                out << "\n";
            }
            out << "    };\n\n";
        }
        
        // Generate the opToImmIndices array
        out << "    // Each instruction's immediate value starting index\n";
        out << "    int opToImmIndices[] = {\n";
        for (size_t i = 0; i < opToImmIndices.size(); i++) {
            out << "        " << opToImmIndices[i];
            if (i < opToImmIndices.size() - 1) 
                out << ",";
            out << " // L" << i;
            if (operandCount(opcodes[i]) == 0) {
                out << " - " << opcodes[i] << " (no immediates)";
            } else {
                out << " = " << opToImmIndices[i];
            }
            out << "\n";
        }
        out << "    };\n\n";
        
        // 输出原始操作码地址到新标签索引的映射数组
        out << "    // Mapping array: original opcode addresses\n";
        out << "    int mapping_size = " << opcodes.size() << ";\n";
        out << "    uint32_t orig_addresses[] = {\n";
        for (size_t i = 0; i < opcode_orig_indices.size(); i++) {
            out << "        " << opcode_orig_indices[i];
            if (i < opcode_orig_indices.size() - 1)
                out << ",";
            out << "\n";
        }
        out << "    };\n\n";
        
        // Initialize instruction and immediate value indices
        if (!immediateValues.empty()) {
            out << "    int imm_index = -1; // Immediate value index\n";
        }
        out << "\n    // Start execution\n    NEXT;\n\n";
        
        // Generate label handlers for each opcode
        for (size_t i = 0; i < opcodes.size(); i++) {
            out << "L" << i << ": // Opcode " << opcodes[i] << "\n";
            
            switch (opcodes[i]) {
                case DT_ADD:
                    out << "    do_add();\n";
                    break;
                case DT_SUB:
                    out << "    do_sub();\n";
                    break;
                case DT_MUL:
                    out << "    do_mul();\n";
                    break;
                case DT_DIV:
                    out << "    do_div();\n";
                    break;
                case DT_MOD:
                    out << "    do_mod();\n";
                    break;
                case DT_SHL:
                    out << "    do_shl();\n";
                    break;
                case DT_SHR:
                    out << "    do_shr();\n";
                    break;
                case DT_FP_ADD:
                    out << "    do_fp_add();\n";
                    break;
                case DT_FP_SUB:
                    out << "    do_fp_sub();\n";
                    break;
                case DT_FP_MUL:
                    out << "    do_fp_mul();\n";
                    break;
                case DT_FP_DIV:
                    out << "    do_fp_div();\n";
                    break;
                case DT_INC:
                    out << "    do_inc();\n";
                    break;
                case DT_DUP:
                    out << "    do_dup();\n";
                    break;
                case DT_DEC:
                    out << "    do_dec();\n";
                    break;
                case DT_LOD:
                    out << "    imm_index++;\n";
                    out << "    do_lod(immediates[imm_index]);\n";
                    break;
                case DT_STO:
                    out << "    imm_index++;\n";
                    out << "    do_sto(immediates[imm_index]);\n";
                    break;
                case DT_IMMI:
                    out << "    imm_index++;\n";
                    out << "    do_immi(immediates[imm_index]);\n";
                    break;
                case DT_STO_IMMI:
                    out << "    imm_index++;\n";
                    out << "    do_sto_immi(immediates[imm_index], immediates[imm_index+1]);\n";
                    out << "    imm_index++;\n";
                    break;
                case DT_MEMCPY:
                    out << "    imm_index++;\n";
                    out << "    uint32_t dest = immediates[imm_index];\n";
                    out << "    imm_index++;\n";
                    out << "    uint32_t src = immediates[imm_index];\n";
                    out << "    imm_index++;\n";
                    out << "    uint32_t len = immediates[imm_index];\n";
                    out << "    do_memcpy(dest, src, len);\n";
                    break;
                case DT_MEMSET:
                    out << "    imm_index++;\n";
                    out << "    uint32_t dest_addr = immediates[imm_index];\n";
                    out << "    imm_index++;\n";
                    out << "    uint32_t value = immediates[imm_index];\n";
                    out << "    imm_index++;\n";
                    out << "    uint32_t length = immediates[imm_index];\n";
                    out << "    do_memset(dest_addr, value, length);\n";
                    break;
                case DT_GT:
                    out << "    do_gt();\n";
                    break;
                case DT_LT:
                    out << "    do_lt();\n";
                    break;
                case DT_EQ:
                    out << "    do_eq();\n";
                    break;
                case DT_GT_EQ:
                    out << "    do_gt_eq();\n";
                    break;
                case DT_LT_EQ:
                    out << "    do_lt_eq();\n";
                    break;
                case DT_PRINT:
                    out << "    do_print();\n";
                    break;
                case DT_READ_INT:
                    out << "    imm_index++;\n";
                    out << "    do_read_int(immediates[imm_index]);\n";
                    break;
                case DT_FP_PRINT:
                    out << "    do_fp_print();\n";
                    break;
                case DT_FP_READ:
                    out << "    imm_index++;\n";
                    out << "    do_fp_read(immediates[imm_index]);\n";
                    break;
                case DT_Tik:
                    out << "    do_tik();\n";
                    break;
                case DT_RND:
                    out << "    do_rnd();\n";
                    break;
                case DT_SEEK:
                    out << "    imm_index++;\n";
                    out << "    do_seek(immediates[imm_index]);\n";
                    break;
                case DT_JMP:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int jump_offset = immediates[imm_index];\n";
                    out << "        int target_inst = ip + jump_offset;\n";
                    out << "        if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "            ip = target_inst;\n";
                    out << "            imm_index = opToImmIndices[target_inst] - 1;\n";
                    out << "            goto *labels[ip];\n";
                    out << "        } else {\n";
                    out << "            fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "            exit(1);\n";
                    out << "        }\n";
                    out << "    }\n";
                    break;
                case DT_JZ:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int jump_offset = immediates[imm_index];\n";
                    out << "        int target_inst = ip + jump_offset;\n";
                    out << "        if (POP() == 0) {\n";
                    out << "            if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "                ip = target_inst;\n";
                    out << "                imm_index = opToImmIndices[target_inst] - 1;\n";
                    out << "                goto *labels[ip];\n";
                    out << "            } else {\n";
                    out << "                fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "                exit(1);\n";
                    out << "            }\n";
                    out << "        }\n";
                    out << "        NEXT;\n";
                    out << "    }\n";
                    break;
                case DT_JUMP_IF:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int jump_offset = immediates[imm_index];\n";
                    out << "        if (POP() != 0) {\n";
                    out << "            int current_inst = ip;\n";
                    out << "            int target_inst = current_inst + jump_offset;\n";
                    out << "            if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "                ip = target_inst;\n";
                    out << "                imm_index = opToImmIndices[target_inst] - 1;\n";
                    out << "                goto *labels[ip];\n";
                    out << "            } else {\n";
                    out << "                fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "                exit(1);\n";
                    out << "            }\n";
                    out << "        }\n";
                    out << "        NEXT;\n";
                    out << "    }\n";
                    break;
                case DT_IF_ELSE:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int true_offset = immediates[imm_index];\n";
                    out << "        imm_index++;\n";
                    out << "        int false_offset = immediates[imm_index];\n";
                    out << "        int current_inst = ip;\n";
                    out << "        int target_inst;\n";
                    out << "        if (POP() != 0) {\n";
                    out << "            target_inst = current_inst + true_offset;\n";
                    out << "        } else {\n";
                    out << "            target_inst = current_inst + false_offset;\n";
                    out << "        }\n";
                    out << "        if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "            ip = target_inst;\n";
                    out << "            imm_index = opToImmIndices[target_inst] - 1;\n";
                    out << "            goto *labels[ip];\n";
                    out << "        } else {\n";
                    out << "            fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "            exit(1);\n";
                    out << "        }\n";
                    out << "    }\n";
                    break;
                // Updated CALL implementation with two parameters and address mapping
                case DT_CALL:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        uint32_t target_addr = immediates[imm_index];\n";  // 原始目标地址
                    out << "        imm_index++;\n";
                    out << "        uint32_t num_params = immediates[imm_index];\n";
                    out << "        // 调用函数时传递参数\n";
                    out << "        do_call(target_addr, num_params);\n";
                    out << "        uint32_t new_target = (uint32_t)(-1);\n";
                    out << "        for (int j = 0; j < mapping_size; j++) {\n";
                    out << "            if (orig_addresses[j] == target_addr) {\n";
                    out << "                new_target = j;\n";
                    out << "                break;\n";
                    out << "            }\n";
                    out << "        }\n";
                    out << "        if (new_target != (uint32_t)(-1)) {\n";
                    out << "            ip = new_target;\n";
                    out << "            imm_index = opToImmIndices[new_target] - 1;\n";
                    out << "            goto *labels[ip];\n";
                    out << "        } else {\n";
                    out << "            fprintf(stderr, \"Error: Invalid call target address: %u\\n\", target_addr);\n";
                    out << "            exit(1);\n";
                    out << "        }\n";
                    out << "    }\n";
                    break;
                case DT_RET:
                    out << "    {\n";
                    out << "        int return_ip = do_ret();\n";
                    out << "        if (return_ip >= 0 && return_ip < " << opcodes.size() << ") {\n";
                    out << "            ip = return_ip;\n";
                    out << "            imm_index = opToImmIndices[ip] - 1;\n";
                    out << "            NEXT;\n";
                    out << "        } else {\n";
                    out << "            fprintf(stderr, \"Error: Invalid return address\\n\");\n";
                    out << "            exit(1);\n";
                    out << "        }\n";
                    out << "    }\n";
                    break;
                case DT_END:
                    out << "    do_end();\n";
                    out << "    return 0;\n";
                    break;
                default:
                    out << "    fprintf(stderr, \"Unknown opcode encountered: " << opcodes[i] << "\\n\");\n";
                    out << "    exit(1);\n";
                    break;
            }
            
            if (opcodes[i] != DT_END && 
                opcodes[i] != DT_JMP && 
                opcodes[i] != DT_JZ && 
                opcodes[i] != DT_JUMP_IF && 
                opcodes[i] != DT_IF_ELSE &&
                opcodes[i] != DT_CALL) {
                out << "    NEXT;\n";
            }
            
            out << "\n";
        }
        
        out << "    return 0;\n}\n";
        out.close();
        
        std::cout << "C file generated successfully: " << output_filename << std::endl;
        std::string clang_command = "clang -o " + filename + "_compiled_dt" + " " + output_filename;
        std::cout << "Compiling generated C file with clang: " << clang_command << std::endl;
        system(clang_command.c_str());
        std::string exec_command = filename + "_compiled_dt";
        std::cout << "Executing compiled binary: " << exec_command << std::endl;
        if (benchmarkMode) {
            std::cout << "Benchmark mode enabled." << std::endl;
        }
        system(exec_command.c_str());
    }
};

#endif // DIRECTTHREADINGVM_H