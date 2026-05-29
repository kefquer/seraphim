#pragma once

#include "seraphim/lexer.h"
#include "seraphim/ast/builder.h"
#include <unordered_map>

namespace seraphim::ast
{
    class decl;
}
namespace seraphim
{
    class parser final
    {
        public:
            using input_t = lexer::value_type;

        public:
            parser() = delete;
            ~parser() = default;

            parser(const parser&) = delete;
            parser& operator = (const parser&) = delete;
            parser(parser&&) = delete;
            parser& operator = (parser&&) = delete;

            explicit parser(ast::builder& builder) noexcept;

        private:
            bool good();

            void programm();

            void declaration();
            void var_decl();
            void expr();

            void add_expr();
            void mul_expr();

            void unary_expr();
            void primary_expr();
            void paren_expr();
            void literal_expr();
            void id_expr();

        public:
            void operator()(input_t input);

        private:
            lexer m_lexer;
            ast::builder* m_builder;
            std::unordered_map<input_t, ast::decl*> m_symTab;
    };
}
