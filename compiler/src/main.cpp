#include "seraphim/ast.h"
#include "seraphim/lexer.h"
#include "seraphim/parser.h"
#include "seraphim/codegen.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/TargetParser/Triple.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: spc <source.sp> [-o output]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string exeName = "a.out";
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            exeName = argv[++i];
        }
    }

    // Read source
    std::ifstream file(inputFile);
    if (!file) {
        std::cerr << "Cannot open file: " << inputFile << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lexer + Parser
    Lexer::Lexer lexer;
    lexer.reset(source);
    Parser::Parser parser(lexer);
    std::unique_ptr<AST::Programm> program;
    try {
        program = parser.parseProgram();
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    // Collect !include directive
    std::vector<std::string> includes;
    for (const auto& decl : program->declarations) {
        if (auto* inc = dynamic_cast<AST::IncludeDirective*>(decl.get())) {
            includes.push_back(inc->library);
        }
    }

    // Codegen
    Codegen::CodeGenerator codegen;
    auto module = codegen.generate(*program);
    if (!module) {
        std::cerr << "Code generation failed.\n";
        return 1;
    }

    // Initialize only x86
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();

    std::string targetTriple = llvm::sys::getProcessTriple();
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target error: " << error << "\n";
        return 1;
    }

    llvm::Triple triple(targetTriple);
    llvm::TargetOptions opt;
    llvm::TargetMachine* targetMachine = target->createTargetMachine(
        triple, "generic", "", opt, llvm::Reloc::PIC_);

    module->setTargetTriple(triple);

    // Generate obj file
    std::string objFile = "output.o";
    std::error_code ec;
    llvm::raw_fd_ostream dest(objFile, ec);
    if (ec) {
        std::cerr << "Could not open output file: " << ec.message() << "\n";
        return 1;
    }

    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "Target Machine can't emit object file\n";
        return 1;
    }
    pass.run(*module);
    dest.flush();

    // Linking libs with !include
    std::string linkCmd = "clang " + objFile;
    for (const auto& lib : includes) {
        linkCmd += " ../stdlib/" + lib + "/" + lib + ".o";
    }
    linkCmd += " -o " + exeName;

    std::cout << "Linking: " << linkCmd << "\n";
    int result = std::system(linkCmd.c_str());
    if (result != 0) {
        std::cerr << "Linking failed\n";
        return 1;
    }

    std::cout << "Executable created: " << exeName << "\n";
    return 0;
}
