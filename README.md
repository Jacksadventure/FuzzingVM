# Introduction to the ThreadingVM Instruction Set

The `ThreadingVM` class is a virtual machine implementation that uses a direct threading approach to execute a set of predefined instructions. Each instruction performs a specific operation, ranging from arithmetic calculations to memory and flow control. This document introduces each instruction and its functionality within the ThreadingVM.

## Arithmetic Instructions

- **DT_ADD**: Adds the top two unsigned integers on the stack.(verified)
- **DT_SUB**: Subtracts the top unsigned integer from the second top integer.(verified)
- **DT_MUL**: Multiplies the top two unsigned integers.(verified)
- **DT_DIV**: Divides the second top unsigned integer by the top integer.(verified)
- **DT_INC**: Increments the top unsigned integer on the stack.(verified)
- **DT_DEC**: Decrements the top unsigned integer on the stack.(verified)
- **DT_SHL**: Performs a left shift on the second top unsigned integer by the number of bits specified by the top integer.(verified)
- **DT_SHR**: Performs a right shift on the second top unsigned integer by the number of bits specified by the top integer.(verified)
- **DT_FP_ADD**: Adds the top two floating-point numbers.(verified)
- **DT_FP_SUB**: Subtracts the top floating-point number from the second top floating-point number.(verified)
- **DT_FP_MUL**: Multiplies the top two floating-point numbers.(verified)
- **DT_FP_DIV**: Divides the second top floating-point number by the top floating-point number.(verified)

## Memory Control Instructions

- **DT_END**: Clears the stack and resets the instruction pointer.(verified)
- **DT_LOD**: Loads an unsigned integer from the specified memory offset into the stack.(verified)
- **DT_STO**: Stores the top unsigned integer into the memory at the specified offset.(verified)
- **DT_IMMI**: Pushes an immediate unsigned integer value onto the stack.(verified)
- **DT_STO_IMMI**: Stores an immediate unsigned integer into the memory at the specified offset.(verified)
- **DT_MEMCPY**: Copies a block of memory from a source to a destination address.(verified)
- **DT_MEMSET**: Sets a block of memory to a specified value.(verified)

## Flow Control Instructions

- **DT_JMP**: Unconditionally jumps to a specified instruction.
- **DT_JZ**: Jumps to a specified instruction if the top unsigned integer is zero.(verified)
- **DT_IF_ELSE**: Chooses between two branches based on the condition at the top of the stack.(verified)
- **DT_JUMP_IF**: Jumps to a specified instruction if the condition is true.(verified)
- **DT_GT, DT_LT, DT_EQ, DT_GT_EQ, DT_LT_EQ**: Comparison instructions that push 1 (true) or 0 (false) based on the comparison result.(verified)
- **DT_CALL**: Calls a function at a specified address.(verified)
- **DT_RET**: Returns from a function call.(verified)

## Debugging Tools

- **DT_SEEK**: Assign the top of the stack to `debug_num`.(verified)
- **DT_PRINT**: Prints the top unsigned integer on the stack.(verified)
- **DT_FP_PRINT**: Prints the top floating-point number on the stack.(verified)
- **DT_READ_INT**: Reads an integer from standard input and stores it at a specified memory offset.(verified)
- **DT_READ_FP**: Reads a floating-point number from standard input and pushes it onto the stack.(verified)

## How to Run
- **For test**
```bash
mkdir build
cd build
cmake -Dtest=ON ..
make 
make test
```
- **For try it out in main.cpp**
```bash
mkdir build
cd build
cmake -Dbuild=ON ..
make 
./build/main
```
- **Useful tool for generate indirect threading code from direct threading code**
```bash
python3 generate_thread.py
```
