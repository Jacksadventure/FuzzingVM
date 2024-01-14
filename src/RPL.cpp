#include <vector>
#include <iostream>
#include <stack>
#ifndef RPL_H
#define RPL_H
class RPL
{
private:
    uint32_t ip;
    std::vector<uint32_t> instructions;
    std::stack<uint32_t> st;
public:
    RPL():ip(0){};
    ~RPL(){};
    void run_vm(std::vector<std::string> instructions);
};

void RPL::run_vm(std::vector<std::string> instructions){
    for(ip = 0;ip<instructions.size();ip++){
        if(instructions[ip] == "ADD"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a+b);
        }
        else if(instructions[ip] == "SUB"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a-b);
        }
        else if(instructions[ip] == "MUL"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a*b);
        }
        else if(instructions[ip] == "DIV"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a/b);
        }
        else if(instructions[ip] == "SHL"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a<<b);
        }
        else if(instructions[ip] == "SHR"){
            uint32_t a = st.top();
            st.pop();
            uint32_t b = st.top();
            st.pop();
            st.push(a>>b);
        }
        else if(isdigit(instructions[ip][0])){
            st.push(atoi(instructions[ip].c_str()));
        }
        else if(instructions[ip] == "PRINT"){
            std::cout<<st.top()<<std::endl;
            st.pop();
        }
        else if(instructions[ip] == "END"){
            break;
        }
    }
}

#endif