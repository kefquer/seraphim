#pragma once

#include <cstdint>
#include <vector>

namespace seraphim::ast
{
    enum class node_t : std::uint8_t
    {
      List,

      LiteralExpr,
      IdExpr,
      ParenExpr,
      UnaryExpr,
      BinaryExpr,

      VarDecl,
    };

    class node
    {
      public:
        using enum node_t;

      public:
        node() = delete;
        virtual ~node() = default;

        node(const node&) = delete;
        node& operator = (const node&) = delete;
        node(node&&) = delete;
        node& operator = (node&&) = delete;

        explicit node(node_t t) noexcept :
            m_kind{ t }
        {
        }
      public:
          node_t what() const noexcept
          {
              return m_kind;
          }
      private:
          node_t m_kind;
    };

    class list final : public node
    {
        public:
            using data_type = std::vector<const node*>;
            using size_type = data_type::size_type;

        public:
            list() = delete;
            ~list() = default;

            list(const list&) = delete;
            list& operator = (const list&) = delete;
            list(list&&) = delete;
            list& operator = (list&&) = delete;

            explicit list(data_type items) :
                node{ node::List },
                m_data{ std::move(items) }
            {
            }

        public:
            void attach(node& n)
            {
                m_data.push_back(&n);
            }

            const data_type& children() const noexcept
            {
                return m_data;
            }

        private:
            data_type m_data;
    };
}
