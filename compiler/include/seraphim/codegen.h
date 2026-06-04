#pragma once

#include "seraphim/ast.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace Codegen {

    class CodeGenerator {
    public:
        CodeGenerator();
        std::unique_ptr<llvm::Module> generate(const AST::Programm& program);

    private:
        llvm::LLVMContext m_context;
        llvm::IRBuilder<> m_builder;
        std::unique_ptr<llvm::Module> m_module;

        std::unordered_map<std::string, llvm::AllocaInst*> m_namedValues; //----------

        // Generate general func
        void declareExternalFunctions();
        void generateFunction(const AST::FunctionDecl& funcDecl);
        void generateStatement(AST::AstNode* node);
        void predeclareFunctions(const AST::Programm& program);

        // Generate external func
        void generateSproute(AST::SprouteStmt* stmt);
        void generateVariableDecl(AST::VariableDecl* varDecl);
        void generateAssignment(AST::Assignment* assign);
        void generateCallStmt(AST::CallStmt* callStmt);

        // Expressions
        llvm::Value* generateExpression(AST::Expr* expr);
        llvm::Value* generateBinaryOp(AST::BinaryOp* binOp);
        llvm::Value* generateUnaryOp(AST::UnaryOp* unaryOp);
        llvm::Value* generateNumberLiteral(AST::NumberLiteral* lit);
        llvm::Value* generateStringLiteral(AST::StringLiteral* lit);
        llvm::Value* generateBoolLiteral(AST::BoolLiteral* lit);
        llvm::Value* generateIdentifier(AST::Identifier* ident);
        llvm::Value* generateCallExpr(AST::CallExpr* callExpr);

        // Support function
        llvm::Type* getTypeFromName(const std::string& typeName);
        llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    };
}
