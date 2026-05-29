#pragma once
#include "seraphim/ast/base.h"
#include "seraphim/ast/decl.h"
#include "seraphim/token.h"

namespace seraphim::ast
{
    class expr : public node
    {
        public:
            expr() = delete;
            ~expr() = default;

            expr(const expr&) = delete;
            expr& operator = (const expr&) = delete;
            expr(expr&&) = delete;
            expr& operator = (expr&&) = delete;

            explicit expr(node_t t) noexcept :
                node{ t }
            {
            }
    };

    class lit_expr : public expr
    {
        public:
            lit_expr() = delete;
            ~lit_expr() = default;

            lit_expr(const lit_expr&) = delete;
            lit_expr& operator = (const lit_expr&) = delete;
            lit_expr(lit_expr&&) = delete;
            lit_expr& operator = (lit_expr&&) = delete;

            explicit lit_expr(const token& val) noexcept;

        public:
            const token& value() const noexcept;

        private:
            token m_value;
    };

    class id_expr : public expr
    {
        public:
            id_expr() = delete;
            ~id_expr() = default;

            id_expr(const lit_expr&) = delete;
            id_expr& operator = (const id_expr&) = delete;
            id_expr(id_expr&&) = delete;
            id_expr& operator = (id_expr&&) = delete;

            explicit id_expr(decl& declaration) noexcept;

        public:
            const decl& declaration() const noexcept;

            const token& name() const noexcept;

        private:
            decl* m_decl;
    };

    class paren_expr : public expr
    {
        public:
            paren_expr() = delete;
            ~paren_expr() = default;

            paren_expr(const paren_expr&) = delete;
            paren_expr& operator = (const paren_expr&) = delete;
            paren_expr(paren_expr&&) = delete;
            paren_expr& operator = (paren_expr&&) = delete;

            explicit paren_expr(node& internal) noexcept;

        public:
            const node& internal_expr() const noexcept;

        private:
            node* m_expr;
    };

    enum class operation : std::uint8_t
    {
      Unknown,
      UnaryNeg,
      UnaryPos,
      Addition,
      Substraction,
      Multiplication,
      Division
    };

    class unary_expr : public expr
    {
      public:
        unary_expr() = delete;
        ~unary_expr() = default;

        unary_expr(const unary_expr&) = delete;
        unary_expr& operator = (const unary_expr&) = delete;
        unary_expr(unary_expr&&) = delete;
        unary_expr& operator = (unary_expr&&) = delete;

        unary_expr(node& operand, operation op) noexcept;

      public:
          const node& operand() const noexcept;

          operation op() const noexcept;

      private:
          node* m_operand;
          operation m_op;
    };

    class binary_expr : public expr
    {
      public:
        binary_expr() = delete;
        ~binary_expr() = default;

        binary_expr(const binary_expr&) = delete;
        binary_expr& operator = (const binary_expr&) = delete;
        binary_expr(binary_expr&&) = delete;
        binary_expr& operator = (binary_expr&&) = delete;

        binary_expr(node& ls, node& rhs, operation op) noexcept;

      public:
          const node& left() const noexcept;
          const node& right() const noexcept;

          operation op() const noexcept;

      private:
          node* m_lhs;
          node* m_rhs;
          operation m_op;
    };
}
