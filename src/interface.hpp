#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <random> 
class Interface{
    public:
        virtual void run_vm(std::string filename,bool benchmarkMode)=0;
        virtual ~Interface () {};
        int getRandomNumber(int max) {
            std::random_device rd;  // Obtain a random number from hardware
            std::mt19937 eng(rd()); // Seed the generator
            std::uniform_int_distribution<> distr(0,max); // Define the range
            return distr(eng);
        }
};
#endif