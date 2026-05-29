#pragma once

#include "seraphim/token.h"
#include <optional>

namespace seraphim
{
    class lexer final
    {
    public:
        using value_type = token::value_type;
        using iter = value_type::iterator;
        using char_t = value_type::value_type;
        using token_opt = std::optional<token>;

        lexer() noexcept;
        ~lexer() = default;

        lexer(const lexer&) = delete;
        lexer& operator=(const lexer&) = delete;
        lexer(lexer&&) = delete;
        lexer& operator=(lexer&&) = delete;

        void operator()(value_type input) noexcept;

        token next() noexcept;
        const token& peek() noexcept;

    private:
        bool good() const noexcept;
        char_t peek_char() const noexcept;
        void advance() noexcept;
        value_type read_str() const noexcept;
        void skip_spaces() noexcept;

        const token& consume(TokenType type) noexcept;

        const token& punctuation() noexcept;
        const token& op() noexcept;
        const token& number() noexcept;
        const token& identifier() noexcept;

    private:
        value_type m_buffer{};
        iter m_start;
        iter m_end;
        token_opt m_preview{};
    };
}
