#include "interface.hpp"
#ifdef direct
#include "directthreading.cpp"
#endif
#ifdef indirect
#include "indirectthreading.cpp"
#endif
#ifdef routine
#include "routinethreading.cpp"
#endif
#include <memory>
#include <iostream>
int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::unique_ptr<Interface> vm;
    #ifdef direct
    vm = std::make_unique<DirectThreadingVM>();
    #elif indirect
    vm = std::make_unique<IndirectThreadingVM>(); 
    #elif routine
    vm = std::make_unique<RoutineThreadingVM>();
    #endif
    if (!vm) {
        std::cerr << "Virtual machine implementation not initialized." << std::endl;
        return 1;
    }
    vm->run_vm(std::string(argv[1]));
    return 0;
}
