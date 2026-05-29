#pragma once

#include "seraphim/token.h"
#include <string_view>
#include <optional>

namespace Lexer {
    class Lexer final {
        public:
            using CharT     = char;
            using StringT   = std::string_view;
            using Iterator  = StringT::const_iterator;
            using TokenOpt  = std::optional<Token>;

            void reset(StringT source) noexcept;

            Token next() noexcept;

            const Token& peek() noexcept;

        private:
            // States
            StringT     m_source{};
            Iterator    m_start{};
            Iterator    m_current{};
            TokenOpt    m_peek{};

            // Help methods
            bool eof() const noexcept;
            CharT currentChar() const noexcept;
            void advance() noexcept;
            void skip_WhitespaceAndComments() noexcept;

            // Reading lexem
            Token read_Number()                  noexcept;
            Token read_String()                  noexcept;
            Token read_IdentifierOrKeyword()     noexcept;
            Token read_OperatorOrPunctuation()   noexcept;
            Token read_IncludeDirective()        noexcept;

            Token makeToken(TokenType type) const noexcept;

            static TokenType resolveKeyword(StringT word) noexcept;
    };
}
