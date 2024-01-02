#include <iostream>
#include "src/directthreading.cpp"
#include "src/indirectthreading.cpp"
#include "src/symbol.hpp"
uint32_t float_to_uint32(float value) {
    return *reinterpret_cast<uint32_t*>(&value);
}
int main() {
    // DirectThreadingVM vm;
    // // Example program 1: Add 1 and 2, result = 3
    // std::vector<unsigned> program1 = {DT_IMMI, 1, DT_IMMI, 2, DT_ADD, DT_PRINT, DT_END};
    // // Example program 2: Store and Load data in specific memory address, result = 100
    // std::vector<unsigned> program2 = {DT_IMMI,100,DT_STO,128,DT_LOD,128,DT_PRINT,DT_END};
    // // Example program 3: Number comparision(get two number a,b caculate a>b? c+d:e+f;eg.a = 1 b = 2 c=3 d=4 e=5 f=6,result = 11)
    // std::vector<unsigned> program3 = {DT_IMMI,1,DT_IMMI,2,DT_GT,DT_JZ,13,DT_IMMI,3,DT_IMMI,4,DT_JMP,17,DT_IMMI,5,DT_IMMI,6,DT_ADD,DT_PRINT,DT_END};
    // // Example program 4: exchange a and b,result =7
    // std::vector<unsigned> program4 = {DT_IMMI,2,DT_IMMI,1,DT_GT,DT_JZ,13,DT_IMMI,3,DT_IMMI,4,DT_JMP,17,DT_IMMI,5,DT_IMMI,6,DT_ADD,DT_PRINT,DT_END};
    // // Example program 5: sum up from 1 to 100;
    // std::vector<unsigned> program5 = {DT_IMMI,0,DT_STO_IMMI,0,1,DT_LOD,0,DT_ADD,DT_LOD,0,DT_INC,DT_STO,0,DT_LOD,0,DT_IMMI,100,DT_GT,DT_JZ,5,DT_PRINT,DT_END};
    // // Example program 6: 8<<3=1
    // std::vector<unsigned> program6 = {DT_IMMI,8,DT_IMMI,3,DT_SHR,DT_PRINT, DT_END};
    // // Example program 7: print 3
    // std::vector<unsigned> program7 = {DT_IMMI,3,DT_PRINT,DT_END};
    // // Example program 8: print(int) = input 
    // std::vector<unsigned> program8 = {DT_READ_INT,0,DT_LOD,0,DT_PRINT,DT_END};
    // // Example program 9. print 4.5
    // std::vector<unsigned> program9 = {DT_IMMI,float_to_uint32(4.5),DT_FP_PRINT,DT_END};
    // // Example program 10: print(float) = input 
    // std::vector<unsigned> program10 = {DT_FP_READ,0,DT_LOD,0,DT_FP_PRINT,DT_END};
    // // Example program 11: call function, 10+2=12
    // std::vector<unsigned> program11 = {
    //     DT_IMMI, 10,              
    //     DT_CALL, 7, 1,              
    //     DT_SEEK, DT_END,          
    //     DT_IMMI, 2,               
    //     DT_ADD,                  
    //     DT_RET};
    // vm.run_vm(program1);
    // vm.run_vm(program2);
    // vm.run_vm(program3);
    // vm.run_vm(program4);
    // vm.run_vm(program5);
    // vm.run_vm(program6);
    // vm.run_vm(program7);
    // // vm.run_vm(program8);
    // vm.run_vm(program9);
    // // vm.run_vm(program10);
    // vm.run_vm(program11);
    IndirectThreadingVM vm2;
    //Example: add 1+2; result =3; length =7 thread = 1-7 
    std::vector<unsigned> program2_1 = {DT_IMMI, 1, DT_IMMI, 2, DT_ADD, DT_PRINT, DT_END};
    std::vector<unsigned> thread2_1 = {0,2,4,5,6};
    vm2.run_vm(program2_1,thread2_1);
    std::vector<unsigned> program2_2 = {DT_IMMI,100,DT_STO,128,DT_LOD,128,DT_PRINT,DT_END};
    std::vector<unsigned> thread2_2 = {0,2,4,6,7};
    vm2.run_vm(program2_2,thread2_2);
    std::vector<unsigned> program2_3 = { DT_IMMI, 1, DT_IMMI, 2, DT_GT, DT_JZ, 7, DT_IMMI, 3, DT_IMMI, 4, DT_JMP, 9, DT_IMMI, 5, DT_IMMI, 6, DT_ADD, DT_PRINT, DT_END };
    std::vector<unsigned> thread2_3 = {0, 2, 4, 5, 7, 9, 11, 13, 15, 17, 18, 19};
    vm2.run_vm(program2_3,thread2_3);
    // vm2.run_vm(program3);
    // vm2.run_vm(program4);
    // vm2.run_vm(program5);
    // vm2.run_vm(program6);
    // vm2.run_vm(program7);
    // vm2.run_vm(program8);
    // vm2.run_vm(program9);
    // vm2.run_vm(program10);
    // vm2.run_vm(program11);
    return 0;
}
