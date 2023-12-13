cmake_minimum_required(VERSION 3.27)
project(FuzzingVM)

set(CMAKE_CXX_STANDARD 23)

add_executable(Fuzzing main.cpp directthreading.cpp)
add_executable(main_test test/main_test.cpp)
target_link_libraries(main_test main)
add_test(test_main main_test)