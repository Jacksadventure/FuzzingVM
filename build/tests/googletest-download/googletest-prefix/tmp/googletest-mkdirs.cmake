# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-src"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-build"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix/tmp"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix/src/googletest-stamp"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix/src"
  "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix/src/googletest-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-download/googletest-prefix/src/googletest-stamp/${subDir}")
endforeach()
