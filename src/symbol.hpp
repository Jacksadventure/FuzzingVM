// Define the set of instructions supported by the VM
#pragma once
enum Instruction {
    // arithmetic 
    DT_ADD, 
    DT_SUB, 
    DT_MUL,
    DT_DIV,
    DT_SHL, 
    DT_SHR,
    DT_FP_ADD,   
    DT_FP_SUB,   
    DT_FP_MUL,   
    DT_FP_DIV,   
    //memory control
    DT_END,
    DT_LOD,
    DT_STO,
    DT_IMMI,
    DT_INC,
    DT_DEC,
    DT_STO_IMMI,
    DT_MEMCPY,
    DT_MEMSET,
    //flow control
    DT_JMP,
    DT_JZ,
    DT_IF_ELSE,
    DT_JUMP_IF,
    DT_GT,
    DT_LT,
    DT_EQ,
    DT_GT_EQ,
    DT_LT_EQ,
    DT_CALL,
    DT_RET,
    //Debug
    DT_SEEK,
    DT_PRINT,
    DT_READ_INT,
    DT_FP_PRINT,
    DT_FP_READ,
    DT_Tik,
    //System
    DT_SYSCALL,
    DT_RND
};
