#pragma once
#include "seraphim/ast/ast.h"
#include "seraphim/ast/base.h"
#include "seraphim/ast/expr.h"
#include "seraphim/token.h"
#include <concepts>
#include <forward_list>
#include <memory>
#include <vector>

namespace seraphim::ast
{
    class builder final
    {
        public:
            using node_ptr = std::unique_ptr<node>;
            using node_storage = std::forward_list<node_ptr>;
            using node_container = std::vector<node*>;

        public:
            builder() = default;
            ~builder() = default;

            builder(const builder&) = delete;
            builder& operator = (const builder&) = delete;
            builder(builder&&) = delete;
            builder& operator = (builder&&) = delete;

        public:
            const node* root() const noexcept;

            void clear_state() noexcept;

            void make_paren();
            void make_literal(const token& value);
            void make_id(decl& d);
            void make_unary(operation op);
            void make_binary(operation op);

            decl* make_var(const token& name);

            void make_list(list::size_type count);

        private:
            template <ast_node T, typename ...Args> requires std::constructible_from<T, Args...>
            void make(Args&& ...args);

            node* extract() noexcept;

        private:
            node_storage m_nodes;
            node_container m_state;
            node* m_root{};
    };
}
