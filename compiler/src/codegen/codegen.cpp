#include "seraphim/codegen.h"
#include "seraphim/ast.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Verifier.h>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace Codegen {

    CodeGenerator::CodeGenerator()
        :   m_context(),
            m_builder(m_context),
            m_module(std::make_unique<llvm::Module>("Seraphim Module", m_context)) {
    }

    std::unique_ptr<llvm::Module> CodeGenerator::generate(const AST::Programm& programm) {
        for (const auto& decl : programm.declarations) {
            if (auto* func = dynamic_cast<AST::FunctionDecl*>(decl.get())) {
                generateFunction(*func);
            }
        }

        std::string error;
        llvm::raw_string_ostream errorStream(error);
        if (llvm::verifyModule(*m_module, &errorStream)) {
            std::cerr << "Module verification failed: " << error << "\n";
            return nullptr;
        }
        return std::move(m_module);
    }

    void CodeGenerator::generateFunction(const AST::FunctionDecl& funcDecl) {
        llvm::Type* returnType = nullptr;
        if (funcDecl.returnType == "int") {
            returnType = llvm::Type::getInt32Ty(m_context);
        } else if (funcDecl.returnType == "freed") {
            returnType = llvm::Type::getVoidTy(m_context);
        } else {
            std::cerr << "UNknown return type: " << funcDecl.returnType << "\n";
            return;
        }

        std::vector<llvm::Type*> paramTypes;
        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

        llvm::Function* function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, funcDecl.name, m_module.get());

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", function);
        m_builder.SetInsertPoint(entry);

        if (funcDecl.returnType == "freed") {
            m_builder.CreateRetVoid();
        } else {
            m_builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
        }
    }
}
