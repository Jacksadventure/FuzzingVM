# Introduction to the ThreadingVM Instruction Set

The `ThreadingVM` class is a virtual machine implementation that uses a direct threading approach to execute a set of predefined instructions. Each instruction performs a specific operation, ranging from arithmetic calculations to memory and flow control. This document introduces each instruction and its functionality within the ThreadingVM.

---

## Stack State

- **Primary Stack (`st`)**  
  This is the main operational stack where all 32-bit values are stored. It handles both integer values and floating-point values (interpreted via bit-level casting). Most arithmetic, memory, and control flow instructions operate directly on this stack.

- **Stack Collection (`sts`) and Function Calls**  
  When a function call is executed using the **DT_CALL** instruction:
  - The VM pops a specified number of parameters from the current stack and pushes them into a new stack.
  - This new stack is added to the stack collection (`sts`) and becomes the active stack (with `st` always referring to the topmost stack in `sts`).
  - The current instruction pointer (i.e., the return address) is stored in the call stack (`callStack`).
  
  Upon executing a function return (**DT_RET**):
  - The return value is popped from the current stack.
  - The previous stack from the collection is restored.
  - The return value is then pushed onto the restored stack, allowing execution to continue seamlessly.

---

## Immediate Values Details

For instructions that require immediate parameters, the immediate values directly follow the opcode in the instruction stream. The number of immediate values and their purposes vary depending on the instruction. Below are the key details:

### Memory-Related Instructions

- **DT_IMMI**  
  - **Immediate Count: 1**  
  - **Purpose:** Pushes the immediate unsigned integer directly onto the stack.

- **DT_LOD**  
  - **Immediate Count: 1**  
  - **Purpose:** Uses the immediate value as a memory offset to load a 32-bit unsigned integer from memory and push it onto the stack.

- **DT_STO**  
  - **Immediate Count: 1**  
  - **Purpose:** Uses the immediate value as a memory offset to store the top value from the stack into memory.

- **DT_STO_IMMI**  
  - **Immediate Count: 2**  
  - **Purpose:** The first immediate specifies the memory offset, while the second immediate is the value to be stored directly into memory.

- **DT_MEMCPY**  
  - **Immediate Count: 3**  
  - **Purpose:** The three immediates represent the destination address, source address, and length of the memory block to be copied.

- **DT_MEMSET**  
  - **Immediate Count: 3**  
  - **Purpose:** The immediates specify the destination address, the value to set, and the length of the memory block to initialize.

- **DT_READ_INT / DT_READ_FP**  
  - **Immediate Count: 1**  
  - **Purpose:** Specifies the memory address where the input value (integer or floating-point) should be stored after being read from standard input.

### Control Flow Instructions

- **DT_JMP**  
  - **Immediate Count: 1**  
  - **Purpose:** Unconditionally jumps to the instruction located at the target address given by the immediate(reletive offset).

- **DT_JZ**  
  - **Immediate Count: 1**  
  - **Purpose:** Jumps to the specified target address if the top value on the stack is zero(reletive offset).

- **DT_JUMP_IF**  
  - **Immediate Count: 1**  
  - **Purpose:** Pops a condition from the stack and jumps to the target address if the condition is true(reletive offset).

- **DT_IF_ELSE**  
  - **Immediate Count: 2**  
  - **Purpose:** Uses the top-of-stack condition to decide between two branches—jumping to the first immediate (true branch) if the condition is true, or to the second immediate (false branch) if false(reletive offset).

### Function Call-Related Instructions

- **DT_CALL**  
  - **Immediate Count: 2**  
  - **Purpose:**  
    - The first immediate is the address of the function to be called(absolute address).  
    - The second immediate indicates the number of parameters to pop from the current stack and transfer to a new stack for the function.

- **DT_RET**  
  - **Immediate Count: 0**  
  - **Purpose:** Returns from a function call by restoring the previous stack context and pushing the return value onto that stack.

---

## Instruction Categories Detailed Explanation

### 1. Arithmetic Instructions
- **DT_ADD, DT_SUB, DT_MUL, DT_DIV, DT_MOD**  
  These instructions operate on the top two unsigned integers on the stack, performing addition, subtraction, multiplication, and division (with division by zero checked in **DT_DIV**, **DT_MOD**).

- **DT_INC, DT_DEC**  
  These instructions increment or decrement the unsigned integer at the top of the stack.

- **DT_SHL, DT_SHR**  
  These perform left and right bit shifts on the second top unsigned integer using the number of bits specified by the top integer.

- **DT_FP_ADD, DT_FP_SUB, DT_FP_MUL, DT_FP_DIV**  
  These instructions treat the top values of the stack as floating-point numbers (via reinterpretation) and perform the corresponding arithmetic operations.

### 2. Memory Control Instructions
These instructions manage memory access and manipulation:
- **DT_LOD / DT_STO:**  
  Utilize an immediate memory offset to load or store 32-bit unsigned integers.

- **DT_IMMI / DT_STO_IMMI:**  
  Either push an immediate value onto the stack or directly store an immediate value into memory.

- **DT_MEMCPY / DT_MEMSET:**  
  Perform memory block copying and initialization operations.

### 3. Flow Control Instructions
These instructions govern the execution flow of the program:
- **DT_JMP, DT_JZ, DT_JUMP_IF, DT_IF_ELSE:**  
  Adjust the instruction pointer based on conditions and the immediate values to implement unconditional and conditional jumps.

- **DT_CALL / DT_RET:**  
  Manage function calls and returns by switching between different stack contexts and maintaining a call stack to store return addresses.

### 4. Debugging and I/O Instructions
- **DT_SEEK:**  
  Assigns the value at the top of the stack to a debugging variable (`debug_num`), useful for monitoring internal state.

- **DT_PRINT / DT_FP_PRINT:**  
  Print the top value of the stack as an unsigned integer or a floating-point number, respectively.

- **DT_READ_INT / DT_READ_FP:**  
  Read an integer or floating-point number from standard input and store it at a specified memory address.

- **DT_TIK:**  
  Outputs the string "tik", which can be used for timing or debugging purposes.

- **DT_RND:**  
  Pops a value from the stack and pushes a random number within the range [0, popped value) back onto the stack.

---

## How to Run
- **For try it out(-DIMPELETATION choice direct indirect routine ALL)**
```bash
mkdir build
cd build
cmake -DIMPELETATION=ALL ..
make
```
- **Useful tool for generating indirect threading code from direct threading code**
```bash
python3 generate_thread.py
```
- **Tool for generating Token for Token threading**
```bash
python3 compiler.py
```