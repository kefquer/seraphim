#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AST {

    // Base

    class AstNode {
        public:
            virtual ~AstNode() = default;
    };

    using NodePtr = std::unique_ptr<AstNode>;
    using NodeList = std::vector<NodePtr>;

    // Expression

    class Expr : public AstNode {};

    class NumberLiteral : public Expr {
        public:
            int value;
            explicit NumberLiteral(int v) : value(v) {}
    };

    class StringLiteral : public Expr {
        public:
            std::string value;
            bool hasLSuffix;
    };

    class BoolLiteral : public Expr {
        public:
            bool value;
            explicit BoolLiteral(bool v) : value(v) {}
    };

    class Identifier : public Expr {
        public:
            std::string name;
            explicit Identifier(std::string_view n) : name(n) {}
    };

    class BinaryOp : public Expr {
        public:
            std::string op;

            NodePtr left;
            NodePtr right;
    };

    class UnaryOp : public Expr {
        public:
            std::string op;
            NodePtr operand;
    };

    class CallExpr : public Expr {
        public:
            NodePtr callee;
            NodeList arguments;
    };

    class MemberAccess : public Expr {
        public:
            NodePtr object;
            std::string member;
            bool isStatic;
    };

    // Statement

    class Stmt : public AstNode {};

    class VariableDecl : public Stmt {
        public:
            std::string typeName;
            std::string varName;
            bool isTypeAbsolute;
            bool isValueAbsolute;
            NodePtr initializer;
    };

    class Assignment : public Stmt {
        public:
            NodePtr target;
            NodePtr value;
    };

    class ReturnStmt : public Stmt {
        public:
            NodePtr value;
    };

    class IfStmt : public Stmt {
        public:
            NodePtr condition;
            NodeList thenBody;
            NodeList elseBody;
    };

    class WhileStmt : public Stmt {
        public:
            NodePtr condition;
            NodeList body;
    };

    class BreakcoreStmt : public Stmt {};

    class DoneStmt : public Stmt {};

    class SprinpStmt : public Stmt {
    public:
        NodePtr targetVar;
        NodePtr sizeExpr;
        bool arrowMoving;
    };

    class SprouteStmt : public Stmt {
    public:
        NodePtr expression;
        std::string endModifier;
    };

    class CallStmt : public Stmt {
        public:
            NodePtr callExpr;
    };

    class BlockStmt : public Stmt {
        public:
            NodeList statements;
    };

    // Declaration

    class Decl : public AstNode {};

    class Parameter {
        public:
            std::string typeName;
            std::string name;
            bool isAbsolute;
    };

    class FunctionDecl : public Decl {
        public:
            std::string returnType;
            std::string name;
            std::vector<Parameter> params;
            bool isReturnTypeAbsolute;
            bool isFunctionAbsolute;
            NodeList body;
    };

    class VisibilitySection : public AstNode {
        public:
            bool isVisible;
            NodeList members;
    };

    class ClassDecl : public Decl {
        public:
            std::string name;
            std::vector<VisibilitySection> sections;
    };

    class IncludeDirective : public Decl {
        public:
            std::string library;
    };

    class Programm : public AstNode {
        public:
            NodeList declarations;
    };
}
