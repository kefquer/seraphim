#include "seraphim/ast/expr.h"
#include "seraphim/ast/base.h"
#include "seraphim/ast/decl.h"
#include "seraphim/token.h"

namespace seraphim::ast
{
    lit_expr::lit_expr(const token& val) noexcept :
    expr{ node::LiteralExpr },
    m_value{ val }
    {
    }

    const token& lit_expr::value() const noexcept
    {
        return m_value;
    }

    id_expr::id_expr(decl& declaration) noexcept :
        expr{ node::IdExpr },
        m_decl{ &declaration }
    {
    }

    const decl& id_expr::declaration() const noexcept
    {
        return *m_decl;
    }

    const token& id_expr::name() const noexcept
    {
        return m_decl->name();
    }

    paren_expr::paren_expr(node& internal) noexcept :
        expr{ node::ParenExpr },
        m_expr{ &internal }
    {
    }

    const node& paren_expr::internal_expr() const noexcept
    {
        return *m_expr;
    }

    unary_expr::unary_expr(node& operand, operation op) noexcept :
    expr{ node::UnaryExpr },
    m_operand{ &operand },
    m_op{ op }
    {
    }

    const node& unary_expr::operand() const noexcept
    {
        return *m_operand;
    }

    operation unary_expr::op() const noexcept
    {
        return m_op;
    }

    binary_expr::binary_expr(node& lhs, node& rhs, operation op) noexcept :
        expr{ node::BinaryExpr },
        m_lhs{ &lhs },
        m_rhs{ &rhs },
        m_op{ op }
    {
    }

    const node& binary_expr::left() const noexcept
    {
        return *m_lhs;
    }
    const node& binary_expr::right() const noexcept
    {
        return *m_rhs;
    }

    operation binary_expr::op() const noexcept
    {
        return m_op;
    }
}
