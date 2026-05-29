#include "seraphim/ast.h"
#include "seraphim/lexer.h"
#include "seraphim/parser.h"
#include "seraphim/codegen.h"

#include <cstdlib>
#include <exception>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetSelect.h>
#include "llvm/MC/TargetRegistry.h"
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/TargetParser/Triple.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: spc <source.sp>\n";
        return 1;
    }

    // Read source file
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lexer
    Lexer::Lexer lexer;
    lexer.reset(source);

    // Parser
    Parser::Parser parser(lexer);
    std::unique_ptr<AST::Programm> program;
    try {
        program = parser.parseProgram();
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    // Code Generator
    Codegen::CodeGenerator codegen;
    auto module = codegen.generate(*program);
    if (!module) {
        std::cerr << "Code generation failed.\n";
        return 1;
    }

    // Initialize taget
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Architecture
    std::string targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target error: " << error << "\n";
        return 1;
    }

    // Setting machine
    llvm::Triple triple(targetTriple);

    llvm::TargetOptions opt;
    llvm::TargetMachine* targetMachine = target->createTargetMachine(
        triple, "generic", "", opt, llvm::Reloc::PIC_);

    // Set target triple for module
    module->setTargetTriple(triple);

    // Name output object file
    std::string outputObj = "output.o";
    std::error_code ec;
    llvm::raw_fd_ostream dest(outputObj, ec);
    if (ec) {
        std::cerr << "COuld not open output file: " << ec.message() << "\n";
        return 1;
    }

    // Generating obj file
    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "Target Machine can't emit object file\n";
        return 1;
    }
    pass.run(*module);
    dest.flush();

    // Exec sys linker (clang) for get exec file
    std::string exeName = "program";
    std::string linkerCmd = "clang " + outputObj + " -o " + exeName;
    std::cout << "Linking: " << linkerCmd << "\n";
    int linkResolute = std::system(linkerCmd.c_str());
    if (linkResolute != 0) {
        std::cerr << "Linking failed\n";
        return 1;
    }
    std::cout << "Executable created: " << exeName << "\n";
}
