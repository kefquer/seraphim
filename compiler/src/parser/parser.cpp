#include "seraphim/parser.h"
#include "seraphim/ast.h"
#include "seraphim/lexer.h"
#include "seraphim/token.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Parser {

    // Constructor
    Parser::Parser(Lexer::Lexer& lexer) noexcept : m_lexer(lexer) {}

    // Utility tokens
    const Lexer::Token& Parser::peek() const {
        return m_lexer.peek();
    }

    Lexer::Token Parser::advance() {
        return m_lexer.next();
    }

    bool Parser::check(Lexer::TokenType type) const {
        return peek().type() == type;
    }

    bool Parser::match(Lexer::TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    Lexer::Token Parser::consume(Lexer::TokenType type, const char* errorMessage) {
        if (check(type))
            return advance();
        throw std::runtime_error(std::string("Parser error: ") + errorMessage +
                                 " at '" + std::string(peek().value()) + "'");
    }

    // Main entry
    std::unique_ptr<AST::Programm> Parser::parseProgram() {
        auto program = std::make_unique<AST::Programm>();
        while (!check(Lexer::TokenType::Eof)) {
            auto decl = parseDeclaration();
            if (decl)
                program->declarations.push_back(std::move(decl));
        }
        return program;
    }

    // Upper level declarations
    std::unique_ptr<AST::AstNode> Parser::parseDeclaration() {
        if (check(Lexer::TokenType::KwInclude))
            return parseIncludeDirective();

        if (check(Lexer::TokenType::KwClass))
            return parseClassDecl();

        bool isTypeAbsolute = false;
        if (match(Lexer::TokenType::KwAbsolute)) {
            consume(Lexer::TokenType::DoubleColon, "expected '::' after 'absolute'");
            isTypeAbsolute = true;
        }

        std::string typeName;
        if (match(Lexer::TokenType::KwInt)) {
            typeName = "int";
        } else if (match(Lexer::TokenType::KwString)) {
            typeName = "string";
        } else if (match(Lexer::TokenType::KwBool)) {
            typeName = "bool";
        } else if (match(Lexer::TokenType::KwFreed)) {
            typeName = "freed";
        } else if (check(Lexer::TokenType::Colon)) {
            // auto type
        } else {
            throw std::runtime_error("Expected type, freed, or ':' at declaration start");
        }

        consume(Lexer::TokenType::Colon, "expected ':' after type");

        std::string name = std::string(consume(Lexer::TokenType::Identifier, "expected identifier").value());

        if (check(Lexer::TokenType::ParenOpen)) {
            // Function declaration
            auto func = std::make_unique<AST::FunctionDecl>();
            func->name = name;
            func->returnType = typeName.empty() ? "freed" : typeName;
            func->isReturnTypeAbsolute = isTypeAbsolute;
            func->params = parseParameterList();

            func->isFunctionAbsolute = false;
            if (match(Lexer::TokenType::DoubleColon)) {
                if (match(Lexer::TokenType::KwAbsolute)) {
                    func->isFunctionAbsolute = true;
                } else {
                    throw std::runtime_error("expected 'absolute' after '::' on function");
                }
            }

            func->body = std::move(parseBlock()->statements);
            consume(Lexer::TokenType::Semicolon, "expected ';' after function body");
            return func;
        } else {
            // Variable declaration
            auto var = std::make_unique<AST::VariableDecl>();
            var->typeName = typeName;
            var->varName = name;
            var->isTypeAbsolute = isTypeAbsolute;
            var->isValueAbsolute = false;

            if (match(Lexer::TokenType::DoubleColon)) {
                if (match(Lexer::TokenType::KwAbsolute)) {
                    var->isValueAbsolute = true;
                } else {
                    throw std::runtime_error("expected 'absolute' after '::' on variable");
                }
            }

            if (match(Lexer::TokenType::Assign)) {
                var->initializer = parseExpression();
            }

            consume(Lexer::TokenType::Semicolon, "expected ';' after variable declaration");
            return var;
        }
    }

    // Include directive
    std::unique_ptr<AST::IncludeDirective> Parser::parseIncludeDirective() {
        auto token = advance(); // Consumes the whole !include[io] token (lexer specific)
        std::string directive = std::string(token.value());

        auto start = directive.find('[');
        auto end = directive.find(']');
        if (start == std::string::npos || end == std::string::npos || end <= start)
            throw std::runtime_error("Malformed !include directive: " + directive);

        std::string libName = directive.substr(start + 1, end - start - 1);
        auto inc = std::make_unique<AST::IncludeDirective>();
        inc->library = libName;
        return inc;
    }

    // Class declaration
    std::unique_ptr<AST::ClassDecl> Parser::parseClassDecl() {
        consume(Lexer::TokenType::KwClass, "expected 'class'");
        std::string className = std::string(consume(Lexer::TokenType::Identifier, "expected class name").value());
        consume(Lexer::TokenType::BraceOpen, "expected '{' after class name");

        auto classDecl = std::make_unique<AST::ClassDecl>();
        classDecl->name = className;

        while (!check(Lexer::TokenType::BraceClose) && !check(Lexer::TokenType::Eof)) {
            if (check(Lexer::TokenType::KwVisible) || check(Lexer::TokenType::KwInvisible)) {
                classDecl->sections.push_back(parseVisibilitySection());
            } else {
                throw std::runtime_error("expected 'visible:' or 'invisible:' inside class");
            }
        }

        consume(Lexer::TokenType::BraceClose, "expected '}' at end of class");
        consume(Lexer::TokenType::Semicolon, "expected ';' after class definition");
        return classDecl;
    }

    AST::VisibilitySection Parser::parseVisibilitySection() {
        AST::VisibilitySection section;
        section.isVisible = match(Lexer::TokenType::KwVisible);
        if (!section.isVisible)
            consume(Lexer::TokenType::KwInvisible, "expected 'invisible'");

        consume(Lexer::TokenType::Colon, "expected ':' after visibility modifier");

        while (!check(Lexer::TokenType::KwVisible) &&
               !check(Lexer::TokenType::KwInvisible) &&
               !check(Lexer::TokenType::BraceClose) &&
               !check(Lexer::TokenType::Eof)) {
            auto decl = parseDeclaration();
            section.members.push_back(std::move(decl));
        }

        return section;
    }

    // Parameters
    std::vector<AST::Parameter> Parser::parseParameterList() {
        std::vector<AST::Parameter> params;
        consume(Lexer::TokenType::ParenOpen, "expected '(' in function declaration");

        if (!check(Lexer::TokenType::ParenClose)) {
            do {
                params.push_back(parseParameter());
            } while (match(Lexer::TokenType::Comma));
        }

        consume(Lexer::TokenType::ParenClose, "expected ')' after parameters");
        return params;
    }

    AST::Parameter Parser::parseParameter() {
        AST::Parameter param;
        param.isAbsolute = false;

        if (match(Lexer::TokenType::KwAbsolute)) {
            consume(Lexer::TokenType::DoubleColon, "expected '::' after 'absolute'");
            param.isAbsolute = true;
        }

        if (match(Lexer::TokenType::KwInt)) {
            param.typeName = "int";
        } else if (match(Lexer::TokenType::KwString)) {
            param.typeName = "string";
        } else if (match(Lexer::TokenType::KwBool)) {
            param.typeName = "bool";
        } else if (check(Lexer::TokenType::Identifier)) {
            param.typeName = std::string(advance().value());
        } else {
            throw std::runtime_error("expected parameter type");
        }

        consume(Lexer::TokenType::Colon, "expected ':' after parameter type");
        param.name = std::string(consume(Lexer::TokenType::Identifier, "expected parameter name").value());

        return param;
    }

    // Statements
    std::unique_ptr<AST::Stmt> Parser::parseStatement() {
        if (match(Lexer::TokenType::KwBreakcore)) {
            consume(Lexer::TokenType::Semicolon, "expected ';' after 'breakcore'");
            return std::make_unique<AST::BreakcoreStmt>();
        }

        if (match(Lexer::TokenType::KwDone)) {
            consume(Lexer::TokenType::Semicolon, "expected ';' after 'done'");
            return std::make_unique<AST::DoneStmt>();
        }

        if (check(Lexer::TokenType::KwIf))
            return parseIfStmt();

        if (check(Lexer::TokenType::KwWhile))
            return parseWhileStmt();

        if (match(Lexer::TokenType::KwReturn)) {
            auto ret = std::make_unique<AST::ReturnStmt>();
            if (!check(Lexer::TokenType::Semicolon))
                ret->value = parseExpression();
            consume(Lexer::TokenType::Semicolon, "expected ';' after return");
            return ret;
        }

        if (check(Lexer::TokenType::Identifier) && peek().value() == "sprinp")
            return parseSprinp();

        if (check(Lexer::TokenType::Identifier) && peek().value() == "sproute")
            return parseSproute();

        // Variable declaration (built‑in types, absolute, or auto)
        if (check(Lexer::TokenType::KwInt) || check(Lexer::TokenType::KwString) ||
            check(Lexer::TokenType::KwBool) || check(Lexer::TokenType::KwAbsolute) ||
            check(Lexer::TokenType::Colon)) {
            auto decl = parseDeclaration();
            auto varDecl = dynamic_cast<AST::VariableDecl*>(decl.get());
            if (!varDecl)
                throw std::runtime_error("function declaration not allowed inside function");
            decl.release();
            return std::unique_ptr<AST::VariableDecl>(varDecl);
        }

        // Identifier: function call or assignment
        if (check(Lexer::TokenType::Identifier)) {
            auto expr = parsePrimary();               // gives Identifier wrapped in call chain
            expr = parseCallOrMember(std::move(expr));

            if (match(Lexer::TokenType::Assign)) {
                auto value = parseExpression();
                auto assign = std::make_unique<AST::Assignment>();
                assign->target = std::move(expr);
                assign->value = std::move(value);
                consume(Lexer::TokenType::Semicolon, "expected ';' after assignment");
                return assign;
            }

            // Must be a function call statement
            if (auto* callExpr = dynamic_cast<AST::CallExpr*>(expr.get())) {
                auto callStmt = std::make_unique<AST::CallStmt>();
                callStmt->callExpr.reset(callExpr);
                expr.release();
                consume(Lexer::TokenType::Semicolon, "expected ';' after call");
                return callStmt;
            } else {
                throw std::runtime_error("expected function call or assignment after identifier");
            }
        }

        throw std::runtime_error("unexpected token in statement: " + std::string(peek().value()));
    }

    // Blocks
    std::unique_ptr<AST::BlockStmt> Parser::parseBlock() {
        consume(Lexer::TokenType::BraceOpen, "expected '{'");
        auto block = std::make_unique<AST::BlockStmt>();

        while (!check(Lexer::TokenType::BraceClose) && !check(Lexer::TokenType::Eof)) {
            block->statements.push_back(parseStatement());
        }

        consume(Lexer::TokenType::BraceClose, "expected '}'");
        return block;
    }

    // Built‑in I/O statements
    std::unique_ptr<AST::Stmt> Parser::parseSprinp() {
        advance(); // consume 'sprinp'
        consume(Lexer::TokenType::ParenOpen, "expected '(' after sprinp");

        auto stmt = std::make_unique<AST::SprinpStmt>();

        bool hasType = check(Lexer::TokenType::KwInt) || check(Lexer::TokenType::KwString) ||
                       check(Lexer::TokenType::KwBool) || check(Lexer::TokenType::Identifier) ||
                       check(Lexer::TokenType::Colon);
        if (hasType) {
            std::string typeName;
            if (match(Lexer::TokenType::KwInt)) typeName = "int";
            else if (match(Lexer::TokenType::KwString)) typeName = "string";
            else if (match(Lexer::TokenType::KwBool)) typeName = "bool";
            else if (check(Lexer::TokenType::Identifier)) typeName = std::string(advance().value());

            if (!typeName.empty() || check(Lexer::TokenType::Colon)) {
                if (!typeName.empty())
                    consume(Lexer::TokenType::Colon, "expected ':' after type in sprinp variable");
                else
                    advance(); // consume the ':'

                std::string varName = std::string(consume(Lexer::TokenType::Identifier, "expected variable name").value());
                bool isAbsolute = false;
                if (match(Lexer::TokenType::DoubleColon)) {
                    if (match(Lexer::TokenType::KwAbsolute))
                        isAbsolute = true;
                    else
                        throw std::runtime_error("expected 'absolute' after '::' in sprinp variable");
                }

                auto varDecl = std::make_unique<AST::VariableDecl>();
                varDecl->typeName = typeName;
                varDecl->varName = varName;
                varDecl->isTypeAbsolute = false;
                varDecl->isValueAbsolute = isAbsolute;
                varDecl->initializer = nullptr;
                stmt->targetVar = std::move(varDecl);
            }
        } else {
            auto ident = std::make_unique<AST::Identifier>(advance().value());
            stmt->targetVar = std::move(ident);
        }

        if (match(Lexer::TokenType::Comma)) {
            stmt->sizeExpr = parseExpression();
        }

        stmt->arrowMoving = false;
        if (match(Lexer::TokenType::Comma)) {
            if (check(Lexer::TokenType::Identifier)) {
                std::string val = std::string(advance().value());
                if (val == "true" || val == "arrow_moving")
                    stmt->arrowMoving = true;
                else if (val == "false")
                    stmt->arrowMoving = false;
                else
                    throw std::runtime_error("expected 'arrow_moving' or boolean for third parameter of sprinp");
            } else {
                throw std::runtime_error("expected 'arrow_moving' or boolean after second comma in sprinp");
            }
        }

        consume(Lexer::TokenType::ParenClose, "expected ')' after sprinp arguments");
        consume(Lexer::TokenType::Semicolon, "expected ';' after sprinp");
        return stmt;
    }

    std::unique_ptr<AST::Stmt> Parser::parseSproute() {
        advance(); // consume 'sproute'
        consume(Lexer::TokenType::ParenOpen, "expected '(' after sproute");

        auto stmt = std::make_unique<AST::SprouteStmt>();
        stmt->expression = parseExpression();

        if (auto* strLit = dynamic_cast<AST::StringLiteral*>(stmt->expression.get())) {
            if (strLit->hasLSuffix) {
                consume(Lexer::TokenType::Colon, "expected ':' after string with L suffix in sproute");
                if (check(Lexer::TokenType::Identifier)) {
                    stmt->endModifier = std::string(advance().value());
                } else if (check(Lexer::TokenType::StringLiteral)) {
                    std::string raw = std::string(advance().value());
                    if (raw.size() >= 2) {
                        stmt->endModifier = raw.substr(1, raw.size() - 2);
                    }
                } else {
                    throw std::runtime_error("expected modifier after 'L:' in sproute");
                }
            }
        }

        consume(Lexer::TokenType::ParenClose, "expected ')' after sproute arguments");
        consume(Lexer::TokenType::Semicolon, "expected ';' after sproute");
        return stmt;
    }

    std::unique_ptr<AST::Stmt> Parser::parseIfStmt() {
        consume(Lexer::TokenType::KwIf, "expected 'if'");
        consume(Lexer::TokenType::ParenOpen, "expected '(' after if");
        auto condition = parseExpression();
        consume(Lexer::TokenType::ParenClose, "expected ')' after condition");

        auto thenBlock = parseBlock();

        auto ifStmt = std::make_unique<AST::IfStmt>();
        ifStmt->condition = std::move(condition);
        ifStmt->thenBody = std::move(thenBlock->statements);

        if (match(Lexer::TokenType::KwElse)) {
            auto elseBlock = parseBlock();
            ifStmt->elseBody = std::move(elseBlock->statements);
        }

        return ifStmt;
    }

    std::unique_ptr<AST::Stmt> Parser::parseWhileStmt() {
        consume(Lexer::TokenType::KwWhile, "expected 'while'");
        consume(Lexer::TokenType::ParenOpen, "expected '(' after 'while'");
        auto condition = parseExpression();
        consume(Lexer::TokenType::ParenClose, "expected ')' after condition");
        auto bodyBlock = parseBlock();
        auto whileStmt = std::make_unique<AST::WhileStmt>();
        whileStmt->condition = std::move(condition);
        whileStmt->body = std::move(bodyBlock->statements);
        return whileStmt;
    }

    // Expressions
    std::unique_ptr<AST::Expr> Parser::parseExpression() {
        return parseLogicalOr();
    }

    std::unique_ptr<AST::Expr> Parser::parseLogicalOr() {
        auto left = parseLogicalAnd();
        while (match(Lexer::TokenType::LogicalOr)) {
            auto op = std::make_unique<AST::BinaryOp>();
            op->op = "||";
            op->left = std::move(left);
            op->right = parseLogicalAnd();
            left = std::move(op);
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseLogicalAnd() {
        auto left = parseEquality();
        while (match(Lexer::TokenType::LogicalAnd)) {
            auto op = std::make_unique<AST::BinaryOp>();
            op->op = "&&";
            op->left = std::move(left);
            op->right = parseEquality();
            left = std::move(op);
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseEquality() {
        auto left = parseComparison();
        while (true) {
            if (match(Lexer::TokenType::Equal)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "==";
                op->left = std::move(left);
                op->right = parseComparison();
                left = std::move(op);
            } else if (match(Lexer::TokenType::NotEqual)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "!=";
                op->left = std::move(left);
                op->right = parseComparison();
                left = std::move(op);
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseComparison() {
        auto left = parseAddition();
        while (true) {
            if (match(Lexer::TokenType::LessThan)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "<";
                op->left = std::move(left);
                op->right = parseAddition();
                left = std::move(op);
            } else if (match(Lexer::TokenType::GreaterThan)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = ">";
                op->left = std::move(left);
                op->right = parseAddition();
                left = std::move(op);
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseAddition() {
        auto left = parseMultiplication();
        while (true) {
            if (match(Lexer::TokenType::Plus)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "+";
                op->left = std::move(left);
                op->right = parseMultiplication();
                left = std::move(op);
            } else if (match(Lexer::TokenType::Minus)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "-";
                op->left = std::move(left);
                op->right = parseMultiplication();
                left = std::move(op);
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseMultiplication() {
        auto left = parseUnary();
        while (true) {
            if (match(Lexer::TokenType::Asterisk)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "*";
                op->left = std::move(left);
                op->right = parseUnary();
                left = std::move(op);
            } else if (match(Lexer::TokenType::Slash)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "/";
                op->left = std::move(left);
                op->right = parseUnary();
                left = std::move(op);
            } else if (match(Lexer::TokenType::Percent)) {
                auto op = std::make_unique<AST::BinaryOp>();
                op->op = "%";
                op->left = std::move(left);
                op->right = parseUnary();
                left = std::move(op);
            } else {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<AST::Expr> Parser::parseUnary() {
        if (match(Lexer::TokenType::Minus)) {
            auto unary = std::make_unique<AST::UnaryOp>();
            unary->op = "-";
            unary->operand = parseUnary();
            return unary;
        }
        if (match(Lexer::TokenType::LogicalNot)) {
            auto unary = std::make_unique<AST::UnaryOp>();
            unary->op = "!";
            unary->operand = parseUnary();
            return unary;
        }
        return parsePrimary();
    }

    std::unique_ptr<AST::Expr> Parser::parsePrimary() {
        if (check(Lexer::TokenType::Number)) {
            int val = std::stoi(std::string(advance().value()));
            return std::make_unique<AST::NumberLiteral>(val);
        }

        if (check(Lexer::TokenType::StringLiteral)) {
            auto token = advance();
            std::string raw = std::string(token.value());
            bool hasL = false;
            if (raw.size() >= 2 && raw.back() == 'L') {
                hasL = true;
                raw = raw.substr(1, raw.size() - 3);
            } else if (raw.size() >= 2) {
                raw = raw.substr(1, raw.size() - 2);
            }
            auto strLit = std::make_unique<AST::StringLiteral>();
            strLit->value = raw;
            strLit->hasLSuffix = hasL;
            return strLit;
        }

        if (check(Lexer::TokenType::BoolLiteral)) {
            bool val = (advance().value() == "true");
            return std::make_unique<AST::BoolLiteral>(val);
        }

        if (check(Lexer::TokenType::Identifier)) {
            auto ident = std::make_unique<AST::Identifier>(advance().value());
            return parseCallOrMember(std::move(ident));
        }

        if (match(Lexer::TokenType::ParenOpen)) {
            auto expr = parseExpression();
            consume(Lexer::TokenType::ParenClose, "expected ')' after expression");
            return expr;
        }

        throw std::runtime_error("unexpected token in expression: " + std::string(peek().value()));
    }

    std::unique_ptr<AST::Expr> Parser::parseCallOrMember(std::unique_ptr<AST::Expr> callee) {
        while (true) {
            if (check(Lexer::TokenType::ParenOpen)) {
                // Function call
                auto call = std::make_unique<AST::CallExpr>();
                call->callee = std::move(callee);
                advance(); // consume '('
                if (!check(Lexer::TokenType::ParenClose)) {
                    do {
                        call->arguments.push_back(parseExpression());
                    } while (match(Lexer::TokenType::Comma));
                }
                consume(Lexer::TokenType::ParenClose, "expected ')' after arguments");
                callee = std::move(call);
            } else if (match(Lexer::TokenType::Colon)) {
                // Instance member access
                std::string member = std::string(consume(Lexer::TokenType::Identifier, "expected member name after ':'").value());
                auto access = std::make_unique<AST::MemberAccess>();
                access->object = std::move(callee);
                access->member = member;
                access->isStatic = false;
                callee = std::move(access);
            } else if (match(Lexer::TokenType::DoubleColon)) {
                // Static member access
                std::string member = std::string(consume(Lexer::TokenType::Identifier, "expected member name after '::'").value());
                auto access = std::make_unique<AST::MemberAccess>();
                access->object = std::move(callee);
                access->member = member;
                access->isStatic = true;   // correct static flag
                callee = std::move(access);
            } else {
                break;
            }
        }
        return callee;
    }

}
