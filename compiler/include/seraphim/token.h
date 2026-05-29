#pragma once

#include "cstdint"
#include "string_view"

namespace Lexer {

    enum class TokenType : std::uint8_t {
        // Service
        Error,
        Eof,

        // Main
        KwAbsolute,
        KwFreed,
        KwVisible,
        KwInvisible,
        KwBreakcore,
        KwDone,
        KwInt,
        KwBool,
        KwString,
        KwClass,
        KwInclude,
        KwIf,
        KwElse,
        KwElseIf,
        KwWhile,
        KwReturn,

        // Literals
        Number,
        StringLiteral, // string literal L
        BoolLiteral,   // bool literal

        // Identifier
        Identifier, // names var, func, classes

        // Operators
        Plus,           // +
        Minus,          // -
        Asterisk,       // *
        Slash,          // /
        Percent,        // %
        Equal,          // ==
        Assign,         // =
        NotEqual,       // !=
        LessThan,       // <
        GreaterThan,    // >
        LessEqual,      // <=
        GreaterEqual,   // >=
        LogicalAnd,     // &&
        LogicalOr,      // ||
        LogicalNot,     // !

        // Separators
        ParenOpen,      // (
        ParenClose,     // )
        BraceOpen,      // {
        BraceClose,     // }
        Colon,          // :
        DoubleColon,    // ::
        Semicolon,      // ;
        Comma,          // ,
        Arrow,          // ->
    };

    class Token final {
        public:
            using ValueType = std::string_view;

            Token() = delete;
            ~Token() = default;

            Token(const Token&) = default;
            Token& operator = (const Token&) = default;
            Token(Token&&) = default;
            Token& operator = (Token&) = default;

            Token(ValueType value, TokenType type) noexcept
                : m_value(value), m_type(type) {}

            ValueType value() const noexcept { return m_value; }
            TokenType type() const noexcept { return m_type; }

            bool is(TokenType t)    const noexcept { return m_type == t; }
            bool isError()          const noexcept { return m_type == TokenType::Error; }
            bool isEof()            const noexcept { return m_type == TokenType::Eof; }

        private:
            ValueType m_value;
            TokenType m_type;
    };
}
