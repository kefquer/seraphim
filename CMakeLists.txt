cmake_minimum_required(VERSION 3.15)
project(SeraphimLanguage LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs core native support target mc passes analysis)

add_library(spc_lib OBJECT
    compiler/src/lexer/lexer.cpp
    compiler/src/parser/parser.cpp
    compiler/src/codegen/codegen.cpp
)

target_include_directories(spc_lib PUBLIC compiler/include)

# ====== Главный исполняемый файл ======
add_executable(spc compiler/src/main.cpp)
target_link_libraries(spc PRIVATE spc_lib)

# ====== Тесты ======
add_executable(parser_tests tests/compiler/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE spc_lib)
