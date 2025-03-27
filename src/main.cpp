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
#ifdef context
#include "contextthreading.cpp"
#endif
#include <memory>
#include <iostream>
int main(int argc, char* argv[]){
    bool isBenchmark = false;
    std::string filename;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--benchmark" && i + 1 < argc) {
            isBenchmark = true;
            filename = argv[++i];
        } else if (filename.empty()) {
            filename = arg;
        }
    }
    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " [--benchmark] <filename>" << std::endl;
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
    #if context
    vm = std::make_unique<ContextThreadingVM>();
    #endif
    if (!vm) {
        std::cerr << "Virtual machine implementation not initialized." << std::endl;
        return 1;
    }
    vm->run_vm(filename,isBenchmark);
    return 0;
}
