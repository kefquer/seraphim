#include "seraphim/codegen.h"
#include "seraphim/ast.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
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
#include <string>
#include <vector>

namespace Codegen {

    CodeGenerator::CodeGenerator()
        : m_context(),
          m_builder(m_context),
          m_module(std::make_unique<llvm::Module>("Seraphim Module", m_context)) {
    }

    // Outside function
    void CodeGenerator::predeclareFunctions(const AST::Programm& program) {
        for (const auto& decl : program.declarations) {
            if (auto* funcDecl = dynamic_cast<AST::FunctionDecl*>(decl.get())) {
                // Type return
                llvm::Type* returnType = nullptr;
                if (funcDecl->returnType == "int")
                    returnType = llvm::Type::getInt32Ty(m_context);
                else if (funcDecl->returnType == "freed")
                    returnType = llvm::Type::getVoidTy(m_context);
                else
                    continue;

                // Type params
                std::vector<llvm::Type*> paramTypes;
                llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

                // Create declare func
                llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcDecl->name, m_module.get());
            }
        }
    }

    void CodeGenerator::declareExternalFunctions() {
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

    // Generate
    std::unique_ptr<llvm::Module> CodeGenerator::generate(const AST::Programm& programm) {
        predeclareFunctions(programm);
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

    // Functions
    void CodeGenerator::generateFunction(const AST::FunctionDecl& funcDecl) { // ПЕРЕПИСАТЬ НАХУЙ
        llvm::Function* function = m_module->getFunction(funcDecl.name);
        if (!function) {
            std::cerr << "Function " << funcDecl.name << " was not predeclared\n";
            return;
        }
        if (!function->isDeclaration()) {
            std::cerr << "Function " << funcDecl.name << " already as a body.\n";
            return;
        }

        m_namedValues.clear();

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", function);
        m_builder.SetInsertPoint(entry);

        for (const auto& stmt : funcDecl.body) {
            generateStatement(stmt.get());
        }

        if (funcDecl.returnType == "freed") {
            m_builder.CreateRetVoid();
        } else if (funcDecl.returnType == "int") {
            m_builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), 0, true));
        } else {
            m_builder.CreateRetVoid();
        }
    }

    // Generate statement
    void CodeGenerator::generateStatement(AST::AstNode* node) {
        if (auto* sp = dynamic_cast<AST::SprouteStmt*>(node)) {
            generateSproute(sp);
        } else if (auto* varDecl = dynamic_cast<AST::VariableDecl*>(node)) {
            generateVariableDecl(varDecl);
        } else if (auto* assign = dynamic_cast<AST::Assignment*>(node)) {
            generateAssignment(assign);
        } else if (auto* callStmt = dynamic_cast<AST::CallStmt*>(node)) {
            generateCallStmt(callStmt);
        }
        // здесь будут добавляться переменные, if, while и т.д.
    }

    // Declaration var
    void CodeGenerator::generateVariableDecl(AST::VariableDecl* varDecl) {
        llvm::Function* currentrFunc = m_builder.GetInsertBlock()->getParent();
        llvm::Type* type = getTypeFromName(varDecl->typeName);
        if (!type) {
            std::cerr << "Unknown type: " << varDecl->typeName << "\n";
            return;
        }

        llvm::AllocaInst* alloca = createEntryBlockAlloca(currentrFunc, varDecl->varName, type);
        m_namedValues[varDecl->varName] = alloca;

        if (varDecl->initializer) {
            llvm::Value* initVal = generateExpression(static_cast<AST::Expr*>(varDecl->initializer.get()));
            m_builder.CreateStore(initVal, alloca);
        }
    }

    // Assignment var
    void CodeGenerator::generateAssignment(AST::Assignment* assign) {
        auto* ident = dynamic_cast<AST::Identifier*>(assign->target.get());
        if (!ident) {
            std::cerr << "Assignment target must be an identifier (for now)\n";
            return;
        }

        auto it = m_namedValues.find(ident->name);
        if (it == m_namedValues.end()) {
            std::cerr << "Variable '" << ident->name << "' not declarated\n";
            return;
        }

        llvm::Value* value = generateExpression(static_cast<AST::Expr*>(assign->value.get()));
        m_builder.CreateStore(value, it->second);
    }

    // Generate Sproute(spio)
    void CodeGenerator::generateSproute(AST::SprouteStmt* stmt) {
        auto* strLit = dynamic_cast<AST::StringLiteral*>(stmt->expression.get());
        if (!strLit) {
            std::cerr << "sproute only supports string literals for now\n";
            return;
        }

        // Global constant for string
        llvm::Constant* strConstant = llvm::ConstantDataArray::getString(m_context, strLit->value, true);
        llvm::GlobalVariable* strVar = new llvm::GlobalVariable(
            *m_module, strConstant->getType(), true,
            llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
        strVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        llvm::Value* strPtr = m_builder.CreateBitCast(strVar, llvm::PointerType::get(m_context, 0));

        // Modifier
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

    // Call func as instruction
    void CodeGenerator::generateCallStmt(AST::CallStmt* callStmt) {
        generateExpression(static_cast<AST::Expr*>(callStmt->callExpr.get()));
    }

    // Expressions
    llvm::Value* CodeGenerator::generateExpression(AST::Expr* expr) {
        if (auto* num = dynamic_cast<AST::NumberLiteral*>(expr)) {
            return generateNumberLiteral(num);
        } else if (auto* str = dynamic_cast<AST::StringLiteral*>(expr)) {
            return generateStringLiteral(str);
        } else if (auto* bl = dynamic_cast<AST::BoolLiteral*>(expr)) {
            return generateBoolLiteral(bl);
        } else if (auto* ident = dynamic_cast<AST::Identifier*>(expr)) {
            return generateIdentifier(ident);
        } else if (auto* bin = dynamic_cast<AST::BinaryOp*>(expr)) {
            return generateBinaryOp(bin);
        } else if (auto* un = dynamic_cast<AST::UnaryOp*>(expr)) {
            return generateUnaryOp(un);
        } else if (auto* call = dynamic_cast<AST::CallExpr*>(expr)) {
            return generateCallExpr(call);
        }
        std::cerr << "Unsupported expression type\n";
        return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(m_context));
    }

    llvm::Value* CodeGenerator::generateNumberLiteral(AST::NumberLiteral* lit) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), lit->value, true);
    }

    llvm::Value* CodeGenerator::generateStringLiteral(AST::StringLiteral* lit) {
        llvm::Constant* strConstant = llvm::ConstantDataArray::getString(m_context, lit->value, true);
        llvm::GlobalVariable* strVar = new llvm::GlobalVariable(
            *m_module, strConstant->getType(), true,
            llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
        strVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
        return m_builder.CreateBitCast(strVar, llvm::PointerType::get(m_context, 0));
    }

    llvm::Value* CodeGenerator::generateBoolLiteral(AST::BoolLiteral* lit) {
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(m_context), lit->value ? 1 : 0);
    }

    llvm::Value* CodeGenerator::generateIdentifier(AST::Identifier* ident) {
        auto it = m_namedValues.find(ident->name);
        if (it == m_namedValues.end()) {
            std::cerr << "Undefined variable: " << ident->name << "\n";
            return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(m_context));
        }
        return m_builder.CreateLoad(it->second->getAllocatedType(), it->second, ident->name.c_str());
    }

    llvm::Value* CodeGenerator::generateBinaryOp(AST::BinaryOp* binOp) {
        llvm::Value* left = generateExpression(static_cast<AST::Expr*>(binOp->left.get()));
        llvm::Value* right = generateExpression(static_cast<AST::Expr*>(binOp->right.get()));

        if (binOp->op == "+") {
            return m_builder.CreateAdd(left, right, "addtmp");
        } else if (binOp->op == "-") {
            return m_builder.CreateSub(left, right, "subtmp");
        } else if (binOp->op == "*") {
            return m_builder.CreateMul(left, right, "multmp");
        } else if (binOp->op == "/") {
            return m_builder.CreateSDiv(left, right, "divtmp");
        } else if (binOp->op == "%") {
            return m_builder.CreateSRem(left, right, "remtmp");
        } else if (binOp->op == "==") {
            return m_builder.CreateICmpEQ(left, right, "eqtmp");
        } else if (binOp->op == "!=") {
            return m_builder.CreateICmpNE(left, right, "neqtmp");
        } else if (binOp->op == "<") {
            return m_builder.CreateICmpSLT(left, right, "lttmp");
        } else if (binOp->op == ">") {
            return m_builder.CreateICmpSGT(left, right, "gttmp");
        } else if (binOp->op == "&&") {
            llvm::Value* leftBool = m_builder.CreateIsNotNull(left, "leftbool");
            llvm::Value* rightBool = m_builder.CreateIsNotNull(right, "righbool");
            return m_builder.CreateLogicalAnd(leftBool, rightBool, "andtmp");
        } else if (binOp->op == "||") {
            llvm::Value* leftBool = m_builder.CreateIsNotNull(left, "leftbool");
            llvm::Value* rightBool = m_builder.CreateIsNotNull(right, "rightbool");
            return m_builder.CreateLogicalOr(leftBool, rightBool, "ortmp");
        }

        std::cerr << "Unknown binary operator: " << binOp->op << "\n";
        return llvm::Constant::getNullValue(left->getType());
    }

    llvm::Value* CodeGenerator::generateUnaryOp(AST::UnaryOp* unaryOp) {
        llvm::Value* operand = generateExpression(static_cast<AST::Expr*>(unaryOp->operand.get()));
        if (unaryOp->op == "-") {
            return m_builder.CreateNeg(operand, "negtmp");
        } else if (unaryOp->op == "!") {
            llvm::Value* boolVal = m_builder.CreateIsNotNull(operand, "negbool");
            return m_builder.CreateNot(boolVal, "nottmp");
        }
        std::cerr << "Unknown unary operator: " << unaryOp->op << "\n";
        return operand;
    }

    llvm::Value* CodeGenerator::generateCallExpr(AST::CallExpr* callExpr) {
        auto* calleeIdent = dynamic_cast<AST::Identifier*>(callExpr->callee.get());
        if (!calleeIdent) {
            std::cerr << "Callee must be an identifier\n";
            return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(m_context));
        }

        std::string funcName = calleeIdent->name;

        // Search func in module
        llvm::Function* func = m_module->getFunction(funcName);
        if (!func) {
            std::cerr << "Function '" << funcName << "' not declared\n";
            return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(m_context));
        }

        // Check arguments
        if (func->arg_size() != callExpr->arguments.size()) {
            std::cerr << "Argument count mismatch for function '" << funcName << "'\n";
            return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(m_context));
        }

        // Generate arguments
        std::vector<llvm::Value*> args;
        for (size_t i = 0; i < callExpr->arguments.size(); ++i) {
            llvm::Value* argValue = generateExpression(static_cast<AST::Expr*>(callExpr->arguments[i].get()));
            if (!argValue) return nullptr;
            args.push_back(argValue);
        }

        // Create instruction calls
        return m_builder.CreateCall(func, args, "calltmp");
    }

    // Supported methods
    llvm::Type* CodeGenerator::getTypeFromName(const std::string& typeName) {
        if (typeName == "int") return llvm::Type::getInt32Ty(m_context);
        if (typeName == "bool") return llvm::Type::getInt1Ty(m_context);
        if (typeName == "string") return llvm::PointerType::get(m_context, 0);
        return nullptr;
    }

    llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type) {
        llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
        return tmpBuilder.CreateAlloca(type, nullptr, name);
    }
}
