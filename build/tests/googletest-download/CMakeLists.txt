cmake_minimum_required(VERSION 3.10)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           main  # Change to 'master' or a newer release tag
  SOURCE_DIR        "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-src"
  BINARY_DIR        "/Users/apple/Desktop/Fuzzingvm/build/tests/googletest-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
