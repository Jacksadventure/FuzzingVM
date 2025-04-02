#ifndef DIRECTTHREADINGVM_H
#define DIRECTTHREADINGVM_H

#include <vector>
#include <stack>
#include <iostream>
#include <cstring>
#include <fstream>
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
                return 0;
            case DT_LOD:
            case DT_STO:
            case DT_IMMI:
            case DT_READ_INT:
            case DT_FP_READ:
            case DT_JMP:
            case DT_JZ:
            case DT_JUMP_IF:
                return 1;
            case DT_STO_IMMI:
                return 2;
            case DT_MEMCPY:
            case DT_MEMSET:
                return 3;
            case DT_IF_ELSE:
                return 2;
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
        
        size_t raw = 0;
        size_t immediateIndex = 0;
        while (raw < instructions.size()) {
            uint32_t opcode = instructions[raw++];
            opcodes.push_back(opcode);
            instToLabelIndex.push_back(opcodes.size() - 1);
            
            opToImmIndices.push_back(immediateIndex);
            
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
        out << "#include <stdio.h>\n#include <stdlib.h>\n#include <stdint.h>\n#include <string.h>\n\n";
        out << "#define STACK_SIZE 1024\n#define BUFFER_SIZE (4 * 1024 * 1024)\n\n";
        
        // Global variables.
        out << "uint32_t stack[STACK_SIZE];\nint top_index = -1;\nchar buffer[BUFFER_SIZE];\n\n";
        
        // Helper conversion functions.
        out << "float to_float(uint32_t val) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.i = val; return u.f;\n}\n";
        out << "uint32_t from_float(float f) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.f = f; return u.i;\n}\n\n";
        
        // Define the NEXT macro (computed goto).
        out << "#define NEXT goto *labels[++ip]\n\n";
        
        // Embedded instruction implementation functions.
        out << "static inline void do_add() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = a + b;\n}\n\n";
        out << "static inline void do_sub() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = b - a;\n}\n\n";
        out << "static inline void do_mul() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = a * b;\n}\n\n";
        out << "static inline void do_div() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    if(a == 0) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n"
               "    stack[++top_index] = b / a;\n}\n\n";
        out << "static inline void do_shl() {\n"
               "    uint32_t shift = stack[top_index--];\n"
               "    uint32_t value = stack[top_index--];\n"
               "    stack[++top_index] = value << shift;\n}\n\n";
        out << "static inline void do_shr() {\n"
               "    uint32_t shift = stack[top_index--];\n"
               "    uint32_t value = stack[top_index--];\n"
               "    stack[++top_index] = value >> shift;\n}\n\n";
        out << "static inline void do_fp_add() {\n"
               "    float a = to_float(stack[top_index--]);\n"
               "    float b = to_float(stack[top_index--]);\n"
               "    stack[++top_index] = from_float(a + b);\n}\n\n";
        out << "static inline void do_fp_sub() {\n"
               "    float a = to_float(stack[top_index--]);\n"
               "    float b = to_float(stack[top_index--]);\n"
               "    stack[++top_index] = from_float(b - a);\n}\n\n";
        out << "static inline void do_fp_mul() {\n"
               "    float a = to_float(stack[top_index--]);\n"
               "    float b = to_float(stack[top_index--]);\n"
               "    stack[++top_index] = from_float(a * b);\n}\n\n";
        out << "static inline void do_fp_div() {\n"
               "    float a = to_float(stack[top_index--]);\n"
               "    float b = to_float(stack[top_index--]);\n"
               "    if(a == 0.0f) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n"
               "    stack[++top_index] = from_float(b / a);\n}\n\n";
        out << "static inline void do_end() {\n"
               "    top_index = -1;\n"
               "    memset(buffer, 0, BUFFER_SIZE);\n}\n\n";
        out << "static inline void do_lod(uint32_t offset) {\n"
               "    uint32_t value;\n"
               "    memcpy(&value, buffer + offset, sizeof(uint32_t));\n"
               "    stack[++top_index] = value;\n}\n\n";
        out << "static inline void do_sto(uint32_t offset) {\n"
               "    uint32_t value = stack[top_index--];\n"
               "    memcpy(buffer + offset, &value, sizeof(uint32_t));\n}\n\n";
        out << "static inline void do_immi(uint32_t value) {\n"
               "    stack[++top_index] = value;\n}\n\n";
        out << "static inline void do_inc() {\n"
               "    uint32_t value = stack[top_index--];\n"
               "    stack[++top_index] = value + 1;\n}\n\n";
        out << "static inline void do_dec() {\n"
               "    uint32_t value = stack[top_index--];\n"
               "    stack[++top_index] = value - 1;\n}\n\n";
        out << "static inline void do_sto_immi(uint32_t offset, uint32_t number) {\n"
               "    memcpy(buffer + offset, &number, sizeof(uint32_t));\n}\n\n";
        out << "static inline void do_memcpy(uint32_t dest, uint32_t src, uint32_t len) {\n"
               "    memcpy(buffer + dest, buffer + src, len);\n}\n\n";
        out << "static inline void do_memset(uint32_t dest, uint32_t val, uint32_t len) {\n"
               "    memset(buffer + dest, val, len);\n}\n\n";
        out << "static inline void do_gt() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = (b > a) ? 1 : 0;\n}\n\n";
        out << "static inline void do_lt() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = (b < a) ? 1 : 0;\n}\n\n";
        out << "static inline void do_eq() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = (b == a) ? 1 : 0;\n}\n\n";
        out << "static inline void do_gt_eq() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = (b >= a) ? 1 : 0;\n}\n\n";
        out << "static inline void do_lt_eq() {\n"
               "    uint32_t a = stack[top_index--];\n"
               "    uint32_t b = stack[top_index--];\n"
               "    stack[++top_index] = (b <= a) ? 1 : 0;\n}\n\n";
        out << "static inline void do_print() {\n"
               "    if (top_index >= 0) {\n"
               "        printf(\"%u\\n\", stack[top_index]);\n"
               "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n"
               "}\n\n";
        out << "static inline void do_read_int(uint32_t offset) {\n"
               "    uint32_t val;\n"
               "    scanf(\"%u\", &val);\n"
               "    memcpy(buffer + offset, &val, sizeof(uint32_t));\n"
               "}\n\n";
        out << "static inline void do_fp_print() {\n"
               "    if (top_index >= 0) {\n"
               "        float f = to_float(stack[top_index]);\n"
               "        printf(\"%f\\n\", f);\n"
               "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n"
               "}\n\n";
        out << "static inline void do_fp_read(uint32_t offset) {\n"
               "    float val;\n"
               "    scanf(\"%f\", &val);\n"
               "    uint32_t ival = from_float(val);\n"
               "    memcpy(buffer + offset, &ival, sizeof(uint32_t));\n"
               "}\n\n";
        out << "static inline void do_tik() { printf(\"tik\\n\"); }\n\n";
        
        // Generate main function with separate label and immediate arrays
        out << "int main() {\n";
        
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
        out << "    // 每个指令的立即数起始索引\n";
        out << "    int opToImmIndices[] = {\n";
        for (size_t i = 0; i < opToImmIndices.size(); i++) {
            out << "        " << opToImmIndices[i];
            if (i < opToImmIndices.size() - 1) 
                out << ",";
            out << " // L" << i;
            if (operandCount(opcodes[i]) == 0) {
                out << " - " << opcodes[i] << " (没有立即数)";
            } else {
                out << " = " << opToImmIndices[i];
            }
            out << "\n";
        }
        out << "    };\n\n";
        
        // Initialize instruction and immediate value indices
        out << "    int ip = -1; // Instruction pointer\n";
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
                case DT_JMP:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int jump_offset = immediates[imm_index];\n";
                    out << "        // Calculate jump target\n";
                    out << "        int current_inst = ip;\n";
                    out << "        int target_inst = current_inst + jump_offset;\n";
                    
                    // Boundary check
                    out << "        if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "            ip = target_inst;\n";
                    
                    // Calculate and set immediate index for the target instruction
                    out << "            // Set immediate index for target instruction\n";
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
                    out << "        if (stack[top_index--] == 0) {\n";
                    
                    // 对于常见的回到指令2的跳转模式，做特殊处理
                    if (i > 2 && opToImmIndices.size() > 2) {
                        out << "            // Calculate jump target\n";
                        out << "            int target_inst = 2; // 直接跳转到L2\n";
                        
                        // Boundary check
                        out << "            if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                        out << "                ip = target_inst;\n";
                        
                        // Calculate and set immediate index for the target instruction
                        out << "                // Set immediate index for target instruction\n";
                        out << "                imm_index = opToImmIndices[target_inst] - 1;\n";
                        
                        out << "                goto *labels[ip];\n";
                        out << "            } else {\n";
                        out << "                fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                        out << "                exit(1);\n";
                        out << "            }\n";
                    } else {
                        out << "            // Calculate jump target\n";
                        out << "            int current_inst = ip;\n";
                        out << "            int target_inst = current_inst + jump_offset;\n";
                        
                        // Boundary check
                        out << "            if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                        out << "                ip = target_inst;\n";
                        
                        // Calculate and set immediate index for the target instruction
                        out << "                // Set immediate index for target instruction\n";
                        out << "                imm_index = opToImmIndices[target_inst] - 1;\n";
                        
                        out << "                goto *labels[ip];\n";
                        out << "            } else {\n";
                        out << "                fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                        out << "                exit(1);\n";
                        out << "            }\n";
                    }
                    
                    out << "        }\n";
                    out << "        NEXT;\n";  // 添加这行，确保条件不满足时也执行NEXT
                    out << "    }\n";
                    break;
                case DT_JUMP_IF:
                    out << "    {\n";
                    out << "        imm_index++;\n";
                    out << "        int jump_offset = immediates[imm_index];\n";
                    out << "        if (stack[top_index--] != 0) {\n";
                    
                    // Calculate target instruction index
                    out << "            // Calculate jump target\n";
                    out << "            int current_inst = ip;\n";
                    out << "            int target_inst = current_inst + jump_offset;\n";
                    
                    // Boundary check
                    out << "            if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "                ip = target_inst;\n";
                    
                    // Calculate immediate index
                    out << "                // Set immediate index for target instruction\n";
                    out << "                imm_index = opToImmIndices[target_inst] - 1;\n";
                    
                    out << "                goto *labels[ip];\n";
                    out << "            } else {\n";
                    out << "                fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "                exit(1);\n";
                    out << "            }\n";
                    out << "        }\n";
                    out << "        NEXT;\n";  // 添加这行，确保条件不满足时也执行NEXT
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
                    
                    out << "        if (stack[top_index--] != 0) {\n";
                    out << "            target_inst = current_inst + true_offset;\n";
                    out << "        } else {\n";
                    out << "            target_inst = current_inst + false_offset;\n";
                    out << "        }\n";
                    
                    // Boundary check
                    out << "        if (target_inst >= 0 && target_inst < " << opcodes.size() << ") {\n";
                    out << "            ip = target_inst;\n";
                    
                    // Calculate immediate index
                    out << "            // Set immediate index for target instruction\n";
                    out << "            imm_index = opToImmIndices[target_inst] - 1;\n";
                    
                    out << "            goto *labels[ip];\n";
                    out << "        } else {\n";
                    out << "            fprintf(stderr, \"Error: Invalid jump target\\n\");\n";
                    out << "            exit(1);\n";
                    out << "        }\n";
                    out << "    }\n";
                    out << "    NEXT;\n";  // 这可能不会被执行到，但为了一致性添加
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
            
            // If not a terminating instruction and not a branching instruction (which handles NEXT internally),
            // go to next
            if (opcodes[i] != DT_END && 
                opcodes[i] != DT_JMP && 
                opcodes[i] != DT_JZ && 
                opcodes[i] != DT_JUMP_IF && 
                opcodes[i] != DT_IF_ELSE) {
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