cmake_minimum_required(VERSION 3.23)
project(ThreadingVM)
set(CMAKE_CXX_STANDARD 23)

set(COMMON_SRC
    src/main.cpp
    src/readfile.cpp)
set(ALL_IMPLEMENTATION direct indirect routine)
option(IMPLEMENTATION "SELECT IMPLEMENTATION" "ALL")

if(build)
    if(IMPLEMENTATION STREQUAL "ALL")
        foreach(impl IN LISTS ALL_IMPLEMENTATION)
            add_executable(thd_vm_${impl} ${COMMON_SRC} src/${impl}threading.cpp)
            target_compile_definitions(thd_vm_${impl} PRIVATE ${impl})
        endforeach()
    else()
        add_executable(thd_vm_${IMPLEMENTATION} ${COMMON_SRC} src/${IMPLEMENTATION}threading.cpp)
        target_compile_definitions(thd_vm_${IMPLEMENTATION} PRIVATE ${IMPLEMENTATION})
    endif()
endif()

if(test)
    add_subdirectory(src)
    enable_testing()
    add_subdirectory(tests)
endif()
