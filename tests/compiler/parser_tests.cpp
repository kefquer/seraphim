#include "seraphim/lexer.h"
#include "seraphim/parser.h"
#include "seraphim/codegen.h"
#include <iostream>
#include <string>
#include <llvm/Support/raw_ostream.h>

int main() {
    std::string source = R"(
        !include [io]

        class TestClass {
            visible:
                int:x::absolute = 42;
                freed:WhileTest() {
                    while (x == 42) {
                        sproute(x);
                        breakcore;
                    }
                };
                freed:hello() {
                    sproute("Hello World"L:end);
                    WhileTest();
                };
            invisible:
                bool:flag = true;
        };

        freed:main() {
            TestClass::hello();

            int:result = 0;
            result = 10 + 20 * 3;
            if (result > 50) {
                sproute("Big"L:end);
            } else {
                sproute("Small"L:end);
            }
            done;
        };
    )";

    Lexer::Lexer lexer;
    lexer.reset(source);

    // Вывод всех токенов для диагностики
    std::cout << "Tokens:\n";
    Lexer::Lexer debugLexer;
    debugLexer.reset(source);
    while (true) {
        auto t = debugLexer.next();
        std::cout << "  " << static_cast<int>(t.type()) << " '" << t.value() << "'\n";
        if (t.isEof()) break;
    }
    std::cout << "--- end tokens ---\n";
    // После этого заново сбросим лексер для парсера
    lexer.reset(source);

    Parser::Parser parser(lexer);

    try {
        auto program = parser.parseProgram();
        std::cout << "Parser test passed! Program has "
                  << program->declarations.size() << " top-level declarations.\n";

        // Простейшая проверка структуры
        for (auto& decl : program->declarations) {
            if (auto* inc = dynamic_cast<AST::IncludeDirective*>(decl.get())) {
                std::cout << "Include: " << inc->library << "\n";
            } else if (auto* cls = dynamic_cast<AST::ClassDecl*>(decl.get())) {
                std::cout << "Class: " << cls->name << " with "
                          << cls->sections.size() << " sections\n";
            } else if (auto* func = dynamic_cast<AST::FunctionDecl*>(decl.get())) {
                std::cout << "Function: " << func->name << " returning "
                          << func->returnType << " with " << func->body.size() << " statements\n";
            }
        }

        Codegen::CodeGenerator cg;
        auto module = cg.generate(*program);
        if (module) {
            std::cout << "LLVM module generated successfully!\n";
            module->print(llvm::outs(), nullptr);  // выведет IR в консоль
        } else {
            std::cerr << "Code generation failed.\n";
            return 1;
        }

    } catch (const std::exception& ex) {
        std::cerr << "Parser error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
