#include <vector>
#include <stack>
#include <iostream>

// Define the set of instructions supported by the VM
enum Instruction {
    //caculate
    DT_ADD,
    DT_SUB,
    DT_MUL,
    DT_DIV,
    DT_END,
    //memory control
    DT_LOD,
    DT_STO,
    DT_IMMI,
    DT_INC,
    DT_DEC,
    DT_STO_IMMI,
    //flow control
    DT_JMP,
    DT_JZ,
    DT_GT,
    DT_LT,
    DT_EQ,
    DT_GT_EQ,
    DT_LT_EQ,
    DT_SEEK
};

class DirectThreadingVM {
private:
    unsigned ip; // Instruction pointer
    std::stack<unsigned> st; // Stack for operations
    std::vector<unsigned> instructions; // Instruction set
    char* buffer; // Memory buffer
    void (DirectThreadingVM::*instructionTable[256])(void); // Function pointer table for instructions
    void write_memory(char* buffer,uint32_t* src, uint32_t offset, uint32_t size) {
        memcpy(buffer + offset, src, size);
    }

    void write_mem32(char* buffer, unsigned val, uint32_t offset) {
        uint32_t buf[1];
        buf[0] = val;
        write_memory(buffer, (uint32_t *)buf, offset, 4);
    }
    void read_memory(char* buffer, uint8_t* dst, uint32_t offset, uint32_t size) {
        memcpy(dst, buffer + offset, size);
    }

    uint32_t read_mem32(char* buffer, uint32_t offset) {
        uint32_t buf[1];
        read_memory(buffer, (uint8_t *)buf, offset, 4);
        return buf[0];
    }
    // Define operations for each instruction
    void do_add() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(a + b);
    }

    void do_sub() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b - a);
    }

    void do_mul() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(a * b);
    }

    void do_div() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        if (b == 0) {
            std::cout << "Division by zero error" << std::endl;
            return;
        }
        st.push(a / b);
    }

    void do_inc() {
        unsigned a = st.top(); st.pop();
        st.push(++a);
    }

    void do_dec() {
        unsigned a = st.top(); st.pop();
        st.push(--a);
    }

    void do_end() {
        st = std::stack<unsigned>();
        instructions = std::vector<unsigned>();
        ip = 0;
    }

    void do_lod() {
        unsigned offset = instructions[++ip];
        uint32_t a = read_mem32(buffer,offset);
        st.push(a);
    }

    void do_sto() {
        unsigned offset = instructions[++ip];
        unsigned a = st.top(); st.pop();
        write_mem32(buffer,a,offset);
    }

    void do_immi() {
        unsigned a = instructions[++ip];
        st.push(a);
    }

    void do_sto_immi() {
        unsigned offset = instructions[++ip];
        unsigned number = instructions[++ip];
        write_mem32(buffer,number,offset);
    }

    void do_jmp() {
        unsigned target = instructions[++ip];
        ip = target - 1;
    }

    void do_jz() {
        unsigned target = instructions[++ip];
        if (st.top() == 0) {
            ip = target - 1;
        }
        st.pop();
    }

    void do_gt() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b > a ? 1 : 0);
    }

    void do_lt() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b < a ? 1 : 0);
    }

    void do_eq() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b == a ? 1 : 0);
    }

    void do_gt_eq() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b >= a ? 1 : 0);
    }

    void do_lt_eq() {
        unsigned a = st.top(); st.pop();
        unsigned b = st.top(); st.pop();
        st.push(b <= a ? 1 : 0);
    }

    void do_seek() {
        if (!st.empty()) {
            std::cout << "Top of stack: " << st.top() << std::endl;
        } else {
            std::cout << "Stack is empty." << std::endl;
        }
    }

    void init_instruction_table() {
        instructionTable[DT_ADD] = &DirectThreadingVM::do_add;
        instructionTable[DT_SUB] = &DirectThreadingVM::do_sub;
        instructionTable[DT_MUL] = &DirectThreadingVM::do_mul;
        instructionTable[DT_DIV] = &DirectThreadingVM::do_div;
        instructionTable[DT_END] = &DirectThreadingVM::do_end;
        instructionTable[DT_LOD] = &DirectThreadingVM::do_lod;
        instructionTable[DT_STO] = &DirectThreadingVM::do_sto;
        instructionTable[DT_IMMI] = &DirectThreadingVM::do_immi;
        instructionTable[DT_STO_IMMI] = &DirectThreadingVM::do_sto_immi;
        instructionTable[DT_INC] = &DirectThreadingVM::do_inc;
        instructionTable[DT_DEC] = &DirectThreadingVM::do_dec;
        instructionTable[DT_JMP] = &DirectThreadingVM::do_jmp;
        instructionTable[DT_JZ] = &DirectThreadingVM::do_jz;
        instructionTable[DT_GT] = &DirectThreadingVM::do_gt;
        instructionTable[DT_LT] = &DirectThreadingVM::do_lt;
        instructionTable[DT_EQ] = &DirectThreadingVM::do_eq;
        instructionTable[DT_GT_EQ] = &DirectThreadingVM::do_gt_eq;
        instructionTable[DT_LT_EQ] = &DirectThreadingVM::do_lt_eq;
        instructionTable[DT_SEEK] = &DirectThreadingVM::do_seek;
    }

public:
    DirectThreadingVM() : ip(0), buffer(new char[4 * 1024 * 1024]) { // Initialize a 4MB buffer
        init_instruction_table();
    }

    ~DirectThreadingVM() {
        delete[] buffer;
    }

    void run_vm(const std::vector<unsigned>& ins) {
        instructions = ins;
        for (ip = 0; ip < instructions.size(); ip++) {
            (this->*instructionTable[instructions[ip]])();
        }
    }
};


