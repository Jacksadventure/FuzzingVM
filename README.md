###Introduction to the DirectThreadingVM Instruction Set
The DirectThreadingVM class is a virtual machine implementation that uses a direct threading approach to execute a set of predefined instructions. Each instruction performs a specific operation, ranging from arithmetic calculations to memory and flow control. This document introduces each instruction and its functionality within the DirectThreadingVM.

Arithmetic Instructions

DT_ADD: Adds the top two values on the stack. The result is pushed back onto the stack.
DT_SUB: Subtracts the top value on the stack from the second value. The result is pushed back onto the stack.
DT_MUL: Multiplies the top two values on the stack. The result is pushed back onto the stack.
DT_DIV: Divides the second value on the stack by the top value. The result is pushed back onto the stack. A check for division by zero is included.
DT_INC: Increments the top value on the stack.
DT_DEC: Decrements the top value on the stack.
Memory Control Instructions

DT_LOD: Loads a value from the memory buffer into the stack. The memory offset is specified in the instruction stream.
DT_STO: Stores the top value on the stack into the memory buffer at the specified offset.
DT_IMMI: Pushes an immediate value onto the stack. The value is specified in the instruction stream.
DT_STO_IMMI: Stores an immediate value into the memory buffer at a specified offset.
DT_END: Resets the stack and the instruction set, effectively ending the current program.
Flow Control Instructions

DT_JMP: Jumps to a specified instruction in the instruction stream.
DT_JZ: Jumps to a specified instruction if the top value on the stack is zero.
DT_GT: Compares the top two values on the stack and pushes 1 if the second value is greater than the top value; otherwise, pushes 0.
DT_LT: Compares the top two values on the stack and pushes 1 if the second value is less than the top value; otherwise, pushes 0.
DT_EQ: Compares the top two values on the stack and pushes 1 if they are equal; otherwise, pushes 0.
DT_GT_EQ: Compares the top two values on the stack and pushes 1 if the second value is greater than or equal to the top value; otherwise, pushes 0.
DT_LT_EQ: Compares the top two values on the stack and pushes 1 if the second value is less than or equal to the top value; otherwise, pushes 0.
DT_SEEK: Outputs the top value on the stack for debugging purposes.
Additional Details

Instruction Pointer (ip): Used to track the current execution point in the instruction stream.
Stack (st): A stack used for performing operations and temporarily storing values.
Memory Buffer (buffer): A buffer for storing and retrieving data.
Function Pointer Table (instructionTable): Maps each instruction to its corresponding function within the DirectThreadingVM.