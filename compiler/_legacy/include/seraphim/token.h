#pragma once

#include <cstdint>
#include <string_view>

namespace seraphim {
    enum class TokenType : std::uint8_t {
        Error,
        Eof,
        Semicolon,

        Plus,
        Minus,
        Asterisk,
        Slash,
        ParenOpen,
        ParenClose,
        EqSign,

        Identifier,
        Number,

        KwVar,
    };

    class token final {
    public:
        using value_type = std::string_view;

        token() = delete;
        ~token() = default;

        token(const token&) = default;
        token& operator=(const token&) = default;
        token(token&&) = default;
        token& operator=(token&&) = default;

        token(value_type val, TokenType tag) noexcept :
            m_value{ val },
            m_tag{ tag }
        {}

        value_type value() const noexcept { return m_value; }
        TokenType what()   const noexcept { return m_tag; }

    private:
        value_type m_value{};
        TokenType  m_tag{};
    };
}
