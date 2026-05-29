#include "seraphim/parser.h"
#include "seraphim/ast/expr.h"
#include "seraphim/token.h"
//#include <iostream>

namespace seraphim
{
    constexpr auto unary_op(TokenType t) noexcept
    {
        switch (t)
        {
            case TokenType::Plus: return ast::operation::UnaryPos;
            case TokenType::Minus: return ast::operation::UnaryNeg;

            default: return ast::operation::Unknown;
        }
    }

    constexpr auto binary_op(TokenType t) noexcept
    {
        switch (t)
        {
            case TokenType::Plus: return ast::operation::Addition;
            case TokenType::Minus: return ast::operation::Substraction;
            case TokenType::Asterisk: return ast::operation::Multiplication;
            case TokenType::Slash: return ast::operation::Division;

            default: return ast::operation::Unknown;
        }
    }

    parser::parser(ast::builder& builder) noexcept :
      m_builder{ &builder }
    {}

    void parser::operator()(input_t input)
    {
        m_lexer(input);
        programm();
    }

    bool parser::good()
    {
        return m_lexer.peek().what() != TokenType::Eof;
    }

    void parser::programm()
    {
        ast::list::size_type count{};
        while (good())
        {
            declaration();
            ++count;
        }

        m_builder->make_list(count);
        //m_builder->clear_state();
    }

    void parser::declaration()
    {
        auto&& next = m_lexer.peek();
        if (next.what() == TokenType::KwVar)
        {
            var_decl();
            if (m_lexer.peek().what() == TokenType::Semicolon)
            {
                m_lexer.next();
            }
            else
            {
                //error
            }
        }
    }

    void parser::var_decl()
    {
        m_lexer.next();
        auto name = m_lexer.next();
        if (name.what() != TokenType::Identifier)
        {
            //error
            return;
        }

        auto eq = m_lexer.next();
        if (eq.what() != TokenType::EqSign)
        {
            //error
            return;
        }

        expr();
        auto var = m_builder->make_var(name);
        m_symTab.try_emplace(name.value(), var);
    }

    void parser::expr()
    {
        add_expr();
    }

    void parser::add_expr()
    {
        mul_expr();
        for(;;)
        {
            auto&& next = m_lexer.peek();
            const auto t = next.what();
            if (t != TokenType::Plus && t != TokenType::Minus) {
                break;
            }

            m_lexer.next();
            mul_expr();
            m_builder->make_binary(binary_op(t));
        }
    }

    void parser::mul_expr()
    {
        unary_expr();
        for(;;)
        {
            auto&& next = m_lexer.peek();
            const auto t = next.what();
            if (t != TokenType::Asterisk && t != TokenType::Slash) {
                break;
            }

            m_lexer.next();
            unary_expr();
            m_builder->make_binary(binary_op(t));
        }
    }

    void parser::unary_expr()
    {
        auto&& next = m_lexer.peek();
        const auto t = next.what();
        if (t != TokenType::Plus && t != TokenType::Minus) {
            primary_expr();
            return;
        }

        m_lexer.next();
        unary_expr();
        m_builder->make_unary(unary_op(t));
    }

    void parser::primary_expr()
    {
        auto&& next = m_lexer.peek();
        const auto t = next.what();
        if (t == TokenType::ParenOpen)
        {
            paren_expr();
        }
        else if (t == TokenType::Number)
        {
            literal_expr();
        }
        else if (t == TokenType::Identifier)
        {
            id_expr();
        }
        else
        {
            //error
        }
    }

    void parser::paren_expr()
    {
        m_lexer.next();
        expr();
        auto closeParen = m_lexer.next();
        if (closeParen.what() != TokenType::ParenClose)
        {
            //error
        }
        else
        {
            m_builder->make_paren();
        }
    }

    void parser::literal_expr()
    {
        auto num = m_lexer.next();
        m_builder->make_literal(num);
    }

    void parser::id_expr()
    {
        auto name = m_lexer.next();
        auto foundVar = m_symTab.find(name.value());
        if (foundVar == m_symTab.end())
        {
            //error
            return;
        }

        auto decl = foundVar->second;
        m_builder->make_id(*decl);
    }
}
