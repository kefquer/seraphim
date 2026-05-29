#pragma once

#include "seraphim/token.h"
#include "seraphim/ast/base.h"

namespace seraphim::ast
{
    class decl : public node
    {
        public:
            decl() = delete;
            ~decl() = default;

            decl(const decl&) = delete;
            decl& operator = (const decl&) = delete;
            decl(decl&&) = delete;
            decl& operator = (decl&&) = delete;

            decl(node_t t, const token& name) noexcept;

        public:
            const token& name() const noexcept;

        private:
            token m_name;
    };

    class var_decl final : public decl
    {
        public:
            var_decl() = delete;
            ~var_decl() = default;

            var_decl(const var_decl&) = delete;
            var_decl& operator = (const var_decl&) = delete;
            var_decl(var_decl&&) = delete;
            var_decl& operator = (var_decl&&) = delete;

            var_decl(const token& name, node& init) noexcept;

        public:
            const node& initialiser() const noexcept;

        private:
            node* m_init;
    };
}
