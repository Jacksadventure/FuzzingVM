#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string> 
class Interface{
    public:
        virtual void run_vm(std::string filename,bool benchmarkMode)=0;
        virtual ~Interface () {};
};
#endif