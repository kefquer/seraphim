#include "seraphim/codegen.h"
#include "seraphim/ast.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include <memory>
#include <vector>

namespace Codegen {

    CodeGenerator::CodeGenerator()
        : m_context(),
          m_builder(m_context),
          m_module(std::make_unique<llvm::Module>("Seraphim Module", m_context)) {
    }

    void CodeGenerator::declareExternalFunctions() {
        // void __seraphim_sproute(const char* str, const char* modifier)
        llvm::Type* charPtrTy = llvm::PointerType::get(m_context, 0);
        llvm::FunctionType* sprouteTy = llvm::FunctionType::get(
            llvm::Type::getVoidTy(m_context),
            {charPtrTy, charPtrTy},
            false
        );
        if (!m_module->getFunction("__seraphim_sproute")) {
            llvm::Function::Create(sprouteTy, llvm::Function::ExternalLinkage,
                                   "__seraphim_sproute", m_module.get());
        }
    }

    std::unique_ptr<llvm::Module> CodeGenerator::generate(const AST::Programm& programm) {
        declareExternalFunctions();

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
            std::cerr << "Unknown return type: " << funcDecl.returnType << "\n";
            return;
        }

        std::vector<llvm::Type*> paramTypes;
        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

        llvm::Function* function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, funcDecl.name, m_module.get());

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", function);
        m_builder.SetInsertPoint(entry);

        for (const auto& stmt : funcDecl.body) {
            generateStatement(stmt.get());
        }

        if (funcDecl.returnType == "freed") {
            m_builder.CreateRetVoid();
        } else {
            m_builder.CreateRet(llvm::Constant::getNullValue(returnType));
        }
    }

    void CodeGenerator::generateStatement(AST::AstNode* node) {
        if (auto* sp = dynamic_cast<AST::SprouteStmt*>(node)) {
            generateSproute(sp);
        }
        // здесь будут добавляться переменные, if, while и т.д.
    }

    void CodeGenerator::generateSproute(AST::SprouteStmt* stmt) {
        auto* strLit = dynamic_cast<AST::StringLiteral*>(stmt->expression.get());
        if (!strLit) {
            std::cerr << "sproute only supports string literals for now\n";
            return;
        }

        // Глобальная константа для строки
        llvm::Constant* strConstant = llvm::ConstantDataArray::getString(m_context, strLit->value, true);
        llvm::GlobalVariable* strVar = new llvm::GlobalVariable(
            *m_module, strConstant->getType(), true,
            llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
        strVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        llvm::Value* strPtr = m_builder.CreateBitCast(strVar, llvm::PointerType::get(m_context, 0));

        // Модификатор
        llvm::Value* modPtr = nullptr;
        if (strLit->hasLSuffix && !stmt->endModifier.empty()) {
            llvm::Constant* modConstant = llvm::ConstantDataArray::getString(m_context, stmt->endModifier, true);
            llvm::GlobalVariable* modVar = new llvm::GlobalVariable(
                *m_module, modConstant->getType(), true,
                llvm::GlobalValue::PrivateLinkage, modConstant, ".mod");
            modVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            modPtr = m_builder.CreateBitCast(modVar, llvm::PointerType::get(m_context, 0));
        } else {
            modPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(m_context, 0));
        }

        llvm::Function* func = m_module->getFunction("__seraphim_sproute");
        if (!func) {
            std::cerr << "Function __seraphim_sproute not declared\n";
            return;
        }
        m_builder.CreateCall(func, {strPtr, modPtr});
    }

}
