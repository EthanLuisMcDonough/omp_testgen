cmake_minimum_required(VERSION 3.20.0)
project(test-mangler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SHARED_INSTALL_DIR "" CACHE PATH "Shared installation directory")
set(TM_CLANG_INSTALL_DIR "${SHARED_INSTALL_DIR}" CACHE PATH "Clang installation directory")
set(TM_MLIR_INSTALL_DIR "${SHARED_INSTALL_DIR}" CACHE PATH "MLIR installation directory")
set(TM_FLANG_INSTALL_DIR "${SHARED_INSTALL_DIR}" CACHE PATH "Flang installation directory")

set(TM_CLANG_CMAKE_DIR "${TM_CLANG_INSTALL_DIR}/lib/cmake/clang/")
set(TM_MLIR_CMAKE_DIR "${TM_MLIR_INSTALL_DIR}/lib/cmake/mlir/")
set(TM_FLANG_CMAKE_DIR "${TM_FLANG_INSTALL_DIR}/lib/cmake/flang/")

list(APPEND CMAKE_PREFIX_PATH "${TM_CLANG_CMAKE_DIR};${TM_MLIR_CMAKE_DIR};${TM_FLANG_CMAKE_DIR}")

find_package(Clang REQUIRED HINTS "${TM_CLANG_CMAKE_DIR}")
find_package(MLIR REQUIRED HINTS "${TM_MLIR_CMAKE_DIR}")
find_package(Flang REQUIRED flangFrontend HINTS "${TM_FLANG_CMAKE_DIR}")

include_directories(SYSTEM "${LLVM_INCLUDE_DIRS};${CLANG_INCLUDE_DIRS};${MLIR_INCLUDE_DIRS};${FLANG_INCLUDE_DIRS}")

if(NOT LLVM_ENABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

include(TestBigEndian)
test_big_endian(IS_BIGENDIAN)
if(IS_BIGENDIAN)
    add_compile_definitions(FLANG_BIG_ENDIAN=1)
else()
    add_compile_definitions(FLANG_LITTLE_ENDIAN=1)
endif()

add_library(testMangler
    MODULE
    MangleAction.cpp
    MangleVisitor.cpp
)
target_link_libraries(testMangler
    "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
