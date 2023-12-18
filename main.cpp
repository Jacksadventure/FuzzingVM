#include <iostream>
#include "src/directthreading.cpp"

int main() {
    DirectThreadingVM vm;
    // Example program 1: Add 1 and 2, result = 3
    std::vector<unsigned> program1 = {DT_IMMI, 1, DT_IMMI, 2, DT_ADD, DT_SEEK, DT_END};
    // Example program 2: Store and Load data in specific memory address, result = 100
    std::vector<unsigned> program2 = {DT_IMMI,100,DT_STO,128,DT_LOD,128,DT_SEEK,DT_END};
    // Example program 3: Number comparision(get two number a,b caculate a>b? c+d:e+f;eg.a = 1 b = 2 c=3 d=4 e=5 f=6,result = 11)
    std::vector<unsigned> program3 = {DT_IMMI,1,DT_IMMI,2,DT_GT,DT_JZ,13,DT_IMMI,3,DT_IMMI,4,DT_JMP,17,DT_IMMI,5,DT_IMMI,6,DT_ADD,DT_SEEK,DT_END};
    // Example program 4: exchange a and b,result =7
    std::vector<unsigned> program4 = {DT_IMMI,2,DT_IMMI,1,DT_GT,DT_JZ,13,DT_IMMI,3,DT_IMMI,4,DT_JMP,17,DT_IMMI,5,DT_IMMI,6,DT_ADD,DT_SEEK,DT_END};
    // Example program 5: sum up from 1 to 100;
    std::vector<unsigned> program5 = {DT_IMMI,0,DT_STO_IMMI,0,1,DT_LOD,0,DT_ADD,DT_LOD,0,DT_INC,DT_STO,0,DT_LOD,0,DT_IMMI,100,DT_GT,DT_JZ,5,DT_SEEK,DT_END};
    vm.run_vm(program1);
    vm.run_vm(program2);
    vm.run_vm(program3);
    vm.run_vm(program4);
    vm.run_vm(program5);
    return 0;
}
