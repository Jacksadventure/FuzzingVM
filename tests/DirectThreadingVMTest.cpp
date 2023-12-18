#include <gtest/gtest.h>
#include <vector>
#include "symbol.hpp"
#include "directthreading.cpp"

TEST(Arithmetic, HandlesAddition) {
    std::vector<unsigned> instructions = {DT_IMMI, 5, DT_IMMI, 3, DT_ADD, DT_SEEK, DT_END};
    DirectThreadingVM vm;
    vm.run_vm(instructions);
    EXPECT_EQ(vm.debug_num, 8); 
}

TEST(Arithmetic, HandlesSubtraction) {
    std::vector<unsigned> instructions = {DT_IMMI, 10, DT_IMMI, 4, DT_SUB, DT_SEEK, DT_END};
    DirectThreadingVM vm;
    vm.run_vm(instructions);
    EXPECT_EQ(vm.debug_num, 6);
}


TEST(Arithmetic, HandlesMultiplication) {
    std::vector<uint32_t> instructions = {DT_IMMI, 6, DT_IMMI, 7, DT_MUL, DT_SEEK, DT_END};
    DirectThreadingVM vm;
    vm.run_vm(instructions);
    EXPECT_EQ(vm.debug_num, 42);
}


TEST(Arithmetic, HandlesDivision) {
    std::vector<uint32_t> instructions = {DT_IMMI, 20, DT_IMMI, 5, DT_DIV, DT_SEEK, DT_END};
    DirectThreadingVM vm;
    vm.run_vm(instructions);
    EXPECT_EQ(vm.debug_num, 4);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
