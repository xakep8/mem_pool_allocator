cmake_minimum_required(VERSION 3.13)
project(mem_pool_allocator VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MEM_POOL_ALLOCATOR_STANDALONE ON)
else()
    set(MEM_POOL_ALLOCATOR_STANDALONE OFF)
endif()

option(MEM_POOL_ALLOCATOR_BUILD_EXAMPLE "Build allocator example executable" ${MEM_POOL_ALLOCATOR_STANDALONE})
option(MEM_POOL_ALLOCATOR_BUILD_TESTS "Build allocator tests" ${MEM_POOL_ALLOCATOR_STANDALONE})
option(MEM_POOL_ALLOCATOR_BUILD_BENCHMARKS "Build allocator benchmarks" ${MEM_POOL_ALLOCATOR_STANDALONE})

#---------------Library Compilation---------------

add_library(mem_pool_allocator_lib STATIC
    src/allocator.cpp
    src/allocator_slab.cpp
)

target_include_directories(mem_pool_allocator_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(mem_pool_allocator_lib
    PRIVATE -Wall -Wextra -Wpedantic
)

set_target_properties(mem_pool_allocator_lib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

#-------------------------------------------------

#-----------------Main executable-----------------

if(MEM_POOL_ALLOCATOR_BUILD_EXAMPLE)
    add_executable(${PROJECT_NAME}_executable main.cpp)
    target_link_libraries(${PROJECT_NAME}_executable PRIVATE mem_pool_allocator_lib)

    set_target_properties(${PROJECT_NAME}_executable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    )
endif()

#-------------------------------------------------

#-----------------Test executable-----------------

if(MEM_POOL_ALLOCATOR_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.14.0
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(${PROJECT_NAME}_tests
        tests/test_allocator.cpp
    )

    target_link_libraries(${PROJECT_NAME}_tests
        PRIVATE
        mem_pool_allocator_lib
        GTest::gtest_main
    )

    target_include_directories(${PROJECT_NAME}_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE DEBUG)

    target_compile_options(${PROJECT_NAME}_tests
        PRIVATE -Wall -Wextra -Wpedantic -fsanitize=address
    )

    target_link_options(${PROJECT_NAME}_tests
        PRIVATE -fsanitize=address
    )

    set_target_properties(${PROJECT_NAME}_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/bin
    )
endif()

#-------------------------------------------------

#--------------BenchMark executable---------------

if(MEM_POOL_ALLOCATOR_BUILD_BENCHMARKS)
    add_executable(allocator_bench
        benchmarks/benchmark_allocator.cpp
    )

    target_link_libraries(allocator_bench
      PRIVATE mem_pool_allocator_lib
    )

    target_include_directories(allocator_bench
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_options(allocator_bench
        PRIVATE -O3
    )
    set_target_properties(allocator_bench PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/bin
    )
endif()

#-------------------------------------------------
