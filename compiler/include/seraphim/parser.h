#pragma once

#include "seraphim/lexer.h"
#include "seraphim/token.h"
#include "seraphim/ast.h"
#include <memory>
#include <string>
#include <vector>

namespace Lexer { class Lexer; }

namespace Parser {

    class Parser {
        public:
            explicit Parser(Lexer::Lexer& lexer) noexcept;

            std::unique_ptr<AST::Programm> parseProgram();

        private:
            Lexer::Lexer& m_lexer;

            //      Utilites for tokens

            const Lexer::Token& peek() const;
            Lexer::Token advance();
            bool check(Lexer::TokenType type) const;
            bool match(Lexer::TokenType);
            Lexer::Token consume(Lexer::TokenType type, const char* errorMessage);

            //      Rulles gramar

            // Upper level
            std::unique_ptr<AST::AstNode> parseDeclaration();

            // Directive and Classes
            std::unique_ptr<AST::IncludeDirective> parseIncludeDirective();
            std::unique_ptr<AST::ClassDecl> parseClassDecl();
            AST::VisibilitySection parseVisibilitySection();

            // Function and Var
            std::unique_ptr<AST::FunctionDecl> parseFunctionDecl(
                bool isReturnTypeAbsolute, std::string returnType);
            std::unique_ptr<AST::VariableDecl> parseVariableDecl(
                bool isTypeAbsolute, std::string typeName);
            std::vector<AST::Parameter> parseParameterList();
            AST::Parameter parseParameter();

            // Instructions
            std::unique_ptr<AST::Stmt> parseStatement();
            std::unique_ptr<AST::BlockStmt> parseBlock();
            std::unique_ptr<AST::Stmt> parseSprinp();
            std::unique_ptr<AST::Stmt> parseSproute();
            std::unique_ptr<AST::Stmt> parseIfStmt();
            std::unique_ptr<AST::Stmt> parseWhileStmt();

            // Expretions
            std::unique_ptr<AST::Expr> parseExpression();
            std::unique_ptr<AST::Expr> parseLogicalOr();
            std::unique_ptr<AST::Expr> parseLogicalAnd();
            std::unique_ptr<AST::Expr> parseEquality();
            std::unique_ptr<AST::Expr> parseComparison();
            std::unique_ptr<AST::Expr> parseAddition();
            std::unique_ptr<AST::Expr> parseMultiplication();
            std::unique_ptr<AST::Expr> parseUnary();
            std::unique_ptr<AST::Expr> parsePrimary();
            std::unique_ptr<AST::Expr> parseCallOrMember(std::unique_ptr<AST::Expr> callee);
    };
}
