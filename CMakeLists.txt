cmake_minimum_required(VERSION 3.23)
project(ThreadingVM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g") # 建议加上-g以便调试

set(COMMON_SRC
        src/main.cpp
        src/readfile.cpp)

set(ALL_IMPLEMENTATION direct indirect routine context sw repl)

# 修改这里，不要用 option()
if(NOT DEFINED IMPLEMENTATION)
    set(IMPLEMENTATION "ALL")
endif()

if(IMPLEMENTATION STREQUAL "ALL")
    foreach(impl IN LISTS ALL_IMPLEMENTATION)
        add_executable(thd_vm_${impl} ${COMMON_SRC} src/${impl}threading.cpp)
        target_compile_definitions(thd_vm_${impl} PRIVATE ${impl})
    endforeach()
else()
    add_executable(thd_vm_${IMPLEMENTATION} ${COMMON_SRC} src/${IMPLEMENTATION}threading.cpp)
    target_compile_definitions(thd_vm_${IMPLEMENTATION} PRIVATE ${IMPLEMENTATION})
endif()