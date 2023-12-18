# CMake generated Testfile for 
# Source directory: /Users/apple/Desktop/Fuzzingvm/tests
# Build directory: /Users/apple/Desktop/Fuzzingvm/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[DirectThreadingVMTest]=] "/Users/apple/Desktop/Fuzzingvm/build/tests/DirectThreadingVMTest")
set_tests_properties([=[DirectThreadingVMTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/apple/Desktop/Fuzzingvm/tests/CMakeLists.txt;30;add_test;/Users/apple/Desktop/Fuzzingvm/tests/CMakeLists.txt;0;")
subdirs("googletest-build")
