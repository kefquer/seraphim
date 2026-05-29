#pragma once
#include "seraphim/ast/ast.h"
#include "seraphim/ast/base.h"
#include "seraphim/ast/decl.h"
#include "seraphim/ast/expr.h"

namespace seraphim::ast
{
    template <typename N, typename Visitor>
    concept visitable =
        ast_node<N> &&
        requires(Visitor v, const N* node)
        {
            v.visit(*node);
        };

    template <typename N, typename Visitor>
    concept previewable =
        ast_node<N> &&
        requires(Visitor v, const N* node)
        {
            { v.preview(*node) } -> std::same_as<bool>;
        };

    template <typename Derived>
    class visitor
    {
        public:
            using derived_t = Derived;
            using node_ptr = const node*;
            using node_ref = const node&;

        public:
            visitor() = default;
            ~visitor() = default;

            visitor(const visitor&) = default;
            visitor& operator = (const visitor&) = default;
            visitor(visitor&&) = default;
            visitor& operator = (visitor&&) = default;

            void operator()(node_ptr n)
            {
                visit_root(n);
            }
        private:
            template <ast_node N>
            static auto to(node_ptr n)
            {
                return static_cast<const N*>(n);
            }

            derived_t& to_derived() noexcept
            {
                return static_cast<derived_t&>(*this);
            }

            void visit(const visitable<derived_t> auto* n)
            {
                to_derived().visit(*n);
            }
            void visit(auto)
            {
            }

            bool preview(const previewable<derived_t> auto* n)
            {
                return to_derived().preview(*n);
            }
            bool preview(auto)
            {
                return true;
            }

            void visit_impl(node_ptr)
            {
            }

            void visit_impl(const lit_expr* e)
            {
                preview(e);
                visit(e);
            }

            void visit_impl(const id_expr* e)
            {
                preview(e);
                visit(e);
            }

            void visit_impl(const paren_expr* e)
            {
                if (preview(e))
                    visit_root(&e->internal_expr());

                visit(e);
            }

            void visit_impl(const unary_expr* e)
            {
                if (preview(e))
                    visit_root(&e->operand());

                visit(e);
            }

            void visit_impl(const binary_expr* e)
            {
                if (preview(e))
                {
                    visit_root(&e->left());
                    visit_root(&e->right());
                }

                visit(e);
            }

            void visit_impl(const var_decl* e)
            {
                // Если preview разрешает обход, спускаемся к выражению инициализации переменной
                if (preview(e))
                    visit_root(&e->initialiser());

                visit(e);
            }

            void visit_impl(const list* e)
            {
                if (preview(e))
                {
                    for (auto&& child : e->children())
                    {
                        // Если в векторе лежат указатели, передаем их как есть.
                        // Если объекты, берем их адрес через &.
                        if constexpr (std::is_pointer_v<std::decay_t<decltype(child)>>) {
                            visit_root(child);
                        } else {
                            visit_root(&child);
                        }
                    }
                }

                visit(e);
            }



        private:
            void visit_root(node_ptr n)
            {
                if (!n)
                    return;

                using enum node_t;
                switch (n->what())
                {
                    case LiteralExpr:   visit_impl(to<lit_expr>(n));    break;
                    case IdExpr:        visit_impl(to<id_expr>(n));     break;
                    case ParenExpr:     visit_impl(to<paren_expr>(n));  break;
                    case UnaryExpr:     visit_impl(to<unary_expr>(n));  break;
                    case BinaryExpr:    visit_impl(to<binary_expr>(n)); break;
                    case VarDecl:       visit_impl(to<var_decl>(n));    break;
                    case List:          visit_impl(to<list>(n));        break;
                }
            }
    };
}
