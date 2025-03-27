#ifndef ROUTINETHREADING_H
#define ROUTINETHREADING_H

#include <vector>
#include <stack>
#include <iostream>
#include <cstring>
#include <fstream>
#include <map>
#include "readfile.hpp"
#include "interface.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include "symbol.hpp"

class RoutineThreadingVM : public Interface {
private:
    uint32_t ip; // Instruction pointer
    std::vector<std::stack<uint32_t>> sts; // Collection of operand stacks
    std::stack<uint32_t> st;             // Current operand stack (st = sts.back())
    std::vector<uint32_t> instructions; // Parsed instruction set
    char* buffer;                       // Memory buffer
    std::stack<uint32_t> callStack;     // Call stack

    // Helper functions for conversion between uint32_t and float
    float to_float(uint32_t val) {
        return *reinterpret_cast<float*>(&val);
    }
    uint32_t from_float(float val) {
        return *reinterpret_cast<uint32_t*>(&val);
    }
    
    // Memory operations
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
        read_memory(buffer, reinterpret_cast<uint8_t*>(buf), offset, 4);
        return buf[0];
    }
    
public:
    RoutineThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) {
        sts.push_back(std::stack<uint32_t>());
        st = sts.back();
    }
    ~RoutineThreadingVM() {
        delete[] buffer;
    }
    
    // generateCFile:
    // Generates a pure C source file based solely on the instruction set.
    // No extra commentary is output in the generated main function.
    void run_vm(std::string filename, bool benchmarkMode) {
        try {
            instructions = readFileToUint32Array(filename);
        } catch (const std::exception &e) {
            std::cerr << "Error reading file: " << e.what() << std::endl;
            return;
        }
        std::string output_filename = filename + "_compiled" + ".c";
        std::ofstream out(output_filename);
        if (!out) {
            std::cerr << "Unable to open file " << output_filename << " for writing." << std::endl;
            return;
        }
        // Standard headers and macro definitions
        out << "#include <stdio.h>\n#include <stdlib.h>\n#include <stdint.h>\n#include <string.h>\n\n";
        out << "#define STACK_SIZE 1024\n#define BUFFER_SIZE (4 * 1024 * 1024)\n\n";
        // Global variables: stack, stack pointer, and memory buffer
        out << "uint32_t stack[STACK_SIZE];\nint top_index = -1;\nchar buffer[BUFFER_SIZE];\n\n";
        // Helper functions for conversion between uint32_t and float
        out << "float to_float(uint32_t val) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.i = val; return u.f;\n}\n";
        out << "uint32_t from_float(float f) {\n";
        out << "    union { uint32_t i; float f; } u;\n    u.f = f; return u.i;\n}\n\n";
        // Routine-threading macros and functions
        out << "#define guard(n) asm(\"#\" #n)\n\n";
        out << "void loop_func() {\n    static int count = 100000000;\n";
        out << "    if(count <= 0) exit(0);\n    count--; \n}\n\n";
        // Embedded instruction implementation functions (unchanged)
        out << "void do_add() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n    stack[++top_index] = a + b;\n}\n\n";
        out << "void do_sub() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n    stack[++top_index] = b - a;\n}\n\n";
        out << "void do_mul() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n    stack[++top_index] = a * b;\n}\n\n";
        out << "void do_div() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    if(a == 0) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n";
        out << "    stack[++top_index] = b / a;\n}\n\n";
        out << "void do_shl() {\n    uint32_t shift = stack[top_index--];\n    uint32_t value = stack[top_index--];\n    stack[++top_index] = value << shift;\n}\n\n";
        out << "void do_shr() {\n    uint32_t shift = stack[top_index--];\n    uint32_t value = stack[top_index--];\n    stack[++top_index] = value >> shift;\n}\n\n";
        out << "void do_fp_add() {\n    float a = to_float(stack[top_index--]);\n    float b = to_float(stack[top_index--]);\n";
        out << "    stack[++top_index] = from_float(a + b);\n}\n\n";
        out << "void do_fp_sub() {\n    float a = to_float(stack[top_index--]);\n    float b = to_float(stack[top_index--]);\n";
        out << "    stack[++top_index] = from_float(b - a);\n}\n\n";
        out << "void do_fp_mul() {\n    float a = to_float(stack[top_index--]);\n    float b = to_float(stack[top_index--]);\n";
        out << "    stack[++top_index] = from_float(a * b);\n}\n\n";
        out << "void do_fp_div() {\n    float a = to_float(stack[top_index--]);\n    float b = to_float(stack[top_index--]);\n";
        out << "    if(a == 0.0f) { fprintf(stderr, \"Error: Division by zero\\n\"); exit(1); }\n";
        out << "    stack[++top_index] = from_float(b / a);\n}\n\n";
        out << "void do_end() {\n    top_index = -1;\n    memset(buffer, 0, BUFFER_SIZE);\n}\n\n";
        out << "void do_lod(uint32_t offset) {\n    uint32_t value;\n";
        out << "    memcpy(&value, buffer + offset, sizeof(uint32_t));\n";
        out << "    stack[++top_index] = value;\n}\n\n";
        out << "void do_sto(uint32_t offset) {\n    uint32_t value = stack[top_index--];\n";
        out << "    memcpy(buffer + offset, &value, sizeof(uint32_t));\n}\n\n";
        out << "void do_immi(uint32_t value) {\n    stack[++top_index] = value;\n}\n\n";
        out << "void do_inc() {\n    uint32_t value = stack[top_index--];\n    stack[++top_index] = value + 1;\n}\n\n";
        out << "void do_dec() {\n    uint32_t value = stack[top_index--];\n    stack[++top_index] = value - 1;\n}\n\n";
        out << "void do_sto_immi(uint32_t offset, uint32_t number) {\n";
        out << "    memcpy(buffer + offset, &number, sizeof(uint32_t));\n}\n\n";
        out << "void do_memcpy(uint32_t dest, uint32_t src, uint32_t len) {\n";
        out << "    memcpy(buffer + dest, buffer + src, len);\n}\n\n";
        out << "void do_memset(uint32_t dest, uint32_t val, uint32_t len) {\n";
        out << "    memset(buffer + dest, val, len);\n}\n\n";
        // (Jump-related functions are no longer used in the generated code.)
        // Flow control is now handled by direct goto's.
        out << "void do_gt() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    stack[++top_index] = (b > a) ? 1 : 0;\n}\n\n";
        out << "void do_lt() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    stack[++top_index] = (b < a) ? 1 : 0;\n}\n\n";
        out << "void do_eq() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    stack[++top_index] = (b == a) ? 1 : 0;\n}\n\n";
        out << "void do_gt_eq() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    stack[++top_index] = (b >= a) ? 1 : 0;\n}\n\n";
        out << "void do_lt_eq() {\n    uint32_t a = stack[top_index--];\n    uint32_t b = stack[top_index--];\n";
        out << "    stack[++top_index] = (b <= a) ? 1 : 0;\n}\n\n";
        out << "void do_print() {\n    if (top_index >= 0) {\n";
        out << "        printf(\"%u\\n\", stack[top_index]);\n";
        out << "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n}\n\n";
        out << "void do_read_int(uint32_t offset) {\n    uint32_t val;\n";
        out << "    scanf(\"%u\", &val);\n";
        out << "    memcpy(buffer + offset, &val, sizeof(uint32_t));\n}\n\n";
        out << "void do_fp_print() {\n    if (top_index >= 0) {\n";
        out << "        float f = to_float(stack[top_index]);\n";
        out << "        printf(\"%f\\n\", f);\n";
        out << "    } else { fprintf(stderr, \"Stack is empty.\\n\"); }\n}\n\n";
        out << "void do_fp_read(uint32_t offset) {\n    float val;\n";
        out << "    scanf(\"%f\", &val);\n";
        out << "    uint32_t ival = from_float(val);\n";
        out << "    memcpy(buffer + offset, &ival, sizeof(uint32_t));\n}\n\n";
        out << "void do_tik() { printf(\"tik\\n\"); }\n\n";
        
        // ----------------------
        // Generate main() using labels for each instruction index.
        // ----------------------
        out << "int main() {\n";
        size_t i = 0;
        while (i < instructions.size()) {
            // Emit a label for the current instruction index:
            out << "L" << i << ":\n";
            uint32_t opcode = instructions[i++];
            switch (opcode) {
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
                case DT_END:
                    out << "    do_end();\n";
                    out << "    return 0;\n";
                    break;
                case DT_LOD: {
                    if (i < instructions.size()) {
                        uint32_t operand = instructions[i++];
                        out << "    do_lod(" << operand << ");\n";
                    } else {
                        out << "    /* Error: missing operand for DT_LOD */\n";
                    }
                } break;
                case DT_STO: {
                    if (i < instructions.size()) {
                        uint32_t operand = instructions[i++];
                        out << "    do_sto(" << operand << ");\n";
                    } else {
                        out << "    /* Error: missing operand for DT_STO */\n";
                    }
                } break;
                case DT_IMMI: {
                    if (i < instructions.size()) {
                        uint32_t operand = instructions[i++];
                        out << "    do_immi(" << operand << ");\n";
                    } else {
                        out << "    /* Error: missing operand for DT_IMMI */\n";
                    }
                } break;
                case DT_INC:
                    out << "    do_inc();\n";
                    break;
                case DT_DEC:
                    out << "    do_dec();\n";
                    break;
                case DT_STO_IMMI: {
                    if (i + 1 < instructions.size()) {
                        uint32_t op1 = instructions[i++];
                        uint32_t op2 = instructions[i++];
                        out << "    do_sto_immi(" << op1 << ", " << op2 << ");\n";
                    } else {
                        out << "    /* Error: missing operands for DT_STO_IMMI */\n";
                    }
                } break;
                case DT_MEMCPY: {
                    if (i + 2 < instructions.size()) {
                        uint32_t op1 = instructions[i++];
                        uint32_t op2 = instructions[i++];
                        uint32_t op3 = instructions[i++];
                        out << "    do_memcpy(" << op1 << ", " << op2 << ", " << op3 << ");\n";
                    } else {
                        out << "    /* Error: missing operands for DT_MEMCPY */\n";
                    }
                } break;
                case DT_MEMSET: {
                    if (i + 2 < instructions.size()) {
                        uint32_t op1 = instructions[i++];
                        uint32_t op2 = instructions[i++];
                        uint32_t op3 = instructions[i++];
                        out << "    do_memset(" << op1 << ", " << op2 << ", " << op3 << ");\n";
                    } else {
                        out << "    /* Error: missing operands for DT_MEMSET */\n";
                    }
                } break;
                case DT_JMP: {
                    if (i < instructions.size()) {
                        uint32_t target = instructions[i++];
                        out << "    goto L" << target << ";\n";
                        continue; // jump; skip default goto
                    } else {
                        out << "    /* Error: missing operand for DT_JMP */\n";
                    }
                } break;
                case DT_JZ: {
                    if (i < instructions.size()) {
                        uint32_t target = instructions[i++];
                        out << "    if (stack[top_index--] == 0) goto L" << target << ";\n";
                    } else {
                        out << "    /* Error: missing operand for DT_JZ */\n";
                    }
                } break;
                case DT_JUMP_IF: {
                    if (i < instructions.size()) {
                        uint32_t target = instructions[i++];
                        out << "    if (stack[top_index--] != 0) goto L" << target << ";\n";
                    } else {
                        out << "    /* Error: missing operand for DT_JUMP_IF */\n";
                    }
                } break;
                case DT_IF_ELSE: {
                    if (i + 1 < instructions.size()) {
                        uint32_t trueBranch = instructions[i++];
                        uint32_t falseBranch = instructions[i++];
                        out << "    if (stack[top_index--] != 0) goto L" << trueBranch << ";\n";
                        out << "    else goto L" << falseBranch << ";\n";
                        continue;
                    } else {
                        out << "    /* Error: missing operands for DT_IF_ELSE */\n";
                    }
                } break;
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
                case DT_READ_INT: {
                    if (i < instructions.size()) {
                        uint32_t op = instructions[i++];
                        out << "    do_read_int(" << op << ");\n";
                    } else {
                        out << "    /* Error: missing operand for DT_READ_INT */\n";
                    }
                } break;
                case DT_FP_PRINT:
                    out << "    do_fp_print();\n";
                    break;
                case DT_FP_READ: {
                    if (i < instructions.size()) {
                        uint32_t op = instructions[i++];
                        out << "    do_fp_read(" << op << ");\n";
                    } else {
                        out << "    /* Error: missing operand for DT_FP_READ */\n";
                    }
                } break;
                case DT_Tik:
                    out << "    do_tik();\n";
                    break;
                default:
                    break;
            }
            // If we haven't hit a jump that transfers control, then continue to the next instruction:
            if (i < instructions.size()) {
                out << "    goto L" << i << ";\n";
            }
        }
        out << "    return 0;\n}\n";
        out.close();
        
        std::cout << "C file generated successfully: " << output_filename << std::endl;
        // Compile the generated C file with clang
        std::string clang_command = "clang -o " + filename + "_compiled" + " " + output_filename;
        std::cout << "Compiling generated C file with clang: " << clang_command << std::endl;
        system(clang_command.c_str());
        // Execute the compiled binary
        std::string exec_command = filename + "_compiled";
        std::cout << "Executing compiled binary: " << exec_command << std::endl;
        if (benchmarkMode) {
            std::cout << "Benchmark mode enabled." << std::endl;
        }
        system(exec_command.c_str());
    }
};

#endif // ROUTINETHREADING_H