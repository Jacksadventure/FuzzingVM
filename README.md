# Introduction to the DirectThreadingVM Instruction Set

The `DirectThreadingVM` class is a virtual machine implementation that uses a direct threading approach to execute a set of predefined instructions. Each instruction performs a specific operation, ranging from arithmetic calculations to memory and flow control. This document introduces each instruction and its functionality within the DirectThreadingVM.

## Arithmetic Instructions

- **DT_ADD**: Adds the top two unsigned integers on the stack.
- **DT_SUB**: Subtracts the top unsigned integer from the second top integer.
- **DT_MUL**: Multiplies the top two unsigned integers.
- **DT_DIV**: Divides the second top unsigned integer by the top integer.
- **DT_SHL**: Performs a left shift on the second top unsigned integer by the number of bits specified by the top integer.
- **DT_SHR**: Performs a right shift on the second top unsigned integer by the number of bits specified by the top integer.
- **DT_FP_ADD**: Adds the top two floating-point numbers.
- **DT_FP_SUB**: Subtracts the top floating-point number from the second top floating-point number.
- **DT_FP_MUL**: Multiplies the top two floating-point numbers.
- **DT_FP_DIV**: Divides the second top floating-point number by the top floating-point number.

## Memory Control Instructions

- **DT_END**: Clears the stack and resets the instruction pointer.
- **DT_LOD**: Loads an unsigned integer from the specified memory offset into the stack.
- **DT_STO**: Stores the top unsigned integer into the memory at the specified offset.
- **DT_IMMI**: Pushes an immediate unsigned integer value onto the stack.
- **DT_INC**: Increments the top unsigned integer on the stack.
- **DT_DEC**: Decrements the top unsigned integer on the stack.
- **DT_STO_IMMI**: Stores an immediate unsigned integer into the memory at the specified offset.
- **DT_MEMCPY**: Copies a block of memory from a source to a destination address.
- **DT_MEMSET**: Sets a block of memory to a specified value.

## Flow Control Instructions

- **DT_JMP**: Unconditionally jumps to a specified instruction.
- **DT_JZ**: Jumps to a specified instruction if the top unsigned integer is zero.
- **DT_IF_ELSE**: Chooses between two branches based on the condition at the top of the stack.
- **DT_JUMP_IF**: Jumps to a specified instruction if the condition is true.
- **DT_GT, DT_LT, DT_EQ, DT_GT_EQ, DT_LT_EQ**: Comparison instructions that push 1 (true) or 0 (false) based on the comparison result.
- **DT_CALL**: Calls a function at a specified address.
- **DT_RET**: Returns from a function call.

## Debugging Tools

- **DT_SEEK**: Assign the top of the stack to `debug_num`.
- **DT_PRINT**: Prints the top unsigned integer on the stack.
- **DT_FP_PRINT**: Prints the top floating-point number on the stack.
- **DT_READ_INT**: Reads an integer from standard input and stores it at a specified memory offset.
- **DT_READ_FP**: Reads a floating-point number from standard input and pushes it onto the stack.

## System Calls

- **DT_SYSCALL**: Handles system calls like file operations and process management. It utilizes a switch case to handle different syscall numbers like `SYS_WRITE`, `SYS_READ`, `SYS_OPEN`, `SYS_CLOSE`, and `SYS_LSEEK`.

## How to Run

```bash
mkdir build
cd build
cmake ..
make 
make test
