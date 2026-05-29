#pragma once

#include "seraphim/ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>

namespace Codegen {

    class CodeGenerator {
        public:
            CodeGenerator();

            std::unique_ptr<llvm::Module> generate(const AST::Programm& programm);

        private:
            llvm::LLVMContext m_context;
            llvm::IRBuilder<> m_builder;
            std::unique_ptr<llvm::Module> m_module;

            void generateFunction(const AST::FunctionDecl& funcDecl);
    };
}
