#include "seraphim/ast/builder.h"
#include "seraphim/ast/ast.h"
#include "seraphim/ast/base.h"
#include "seraphim/ast/expr.h"
#include <concepts>
#include <utility>

namespace seraphim::ast
{
    template <ast_node T, typename ...Args>
      requires std::constructible_from<T, Args...>
    void builder::make(Args&& ...args)
    {
        auto newNode = m_nodes.emplace_front(std::make_unique<T>(std::forward<Args>(args)...)).get();
            m_state.push_back(newNode);
            m_root = newNode;
    }

    node* builder::extract() noexcept
    {
        if (m_state.empty())
            return {};

        auto item = m_state.back();
        m_state.pop_back();
        return item;
    }

    const node* builder::root() const noexcept
    {
        return m_root;
    }

    void builder::clear_state() noexcept
    {
        m_state.clear();
    }

    void builder::make_literal(const token& value)
    {
        make<lit_expr>(value);
    }

    void builder::make_id(decl& d)
    {
        make<id_expr>(d);
    }

    void builder::make_paren()
    {
        if (auto internal = extract())
        {
            make<paren_expr>(*internal);
        }
    }

    void builder::make_unary(operation op)
    {
        if (auto operand = extract())
        {
            make<unary_expr>(*operand, op);
        }
    }

    void builder::make_binary(operation op)
    {
        auto rhs = extract();
        auto lhs = extract();
        if (lhs && rhs)
        {
            make<binary_expr>(*lhs, *rhs, op);
        }
    }

    decl* builder::make_var(const token& name)
    {
        auto init = extract();
        if (init)
        {
            make<var_decl>(name, *init);
        }
        else
        {
            //error
            return {};
        }

        return static_cast<decl*>(m_root);
    }

    void builder::make_list(list::size_type count)
    {
        const auto availableSz = static_cast<list::size_type>(m_state.size());
        if (availableSz < count)
        {
            //error
            return;
        }

        const auto startPos = availableSz - count;
        auto beg = std::next(m_state.begin(), startPos);
        list::data_type items;
        items.reserve(count);
        for (auto it = beg; it != m_state.end(); ++it)
        {
            items.push_back(*it);
        }

        m_state.erase(beg, m_state.end());
        make<list>(std::move(items));
    }
}
