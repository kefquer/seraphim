#include "seraphim/lexer.h"
#include "seraphim/token.h"
#include <algorithm>
#include <array>
#include <unordered_map>

namespace seraphim
{
    template <typename It>
    constexpr auto is_in_range(char c, It beg, It end) noexcept {
        return std::find(beg, end, c) != end;
    }

    constexpr auto is_blank(char c) noexcept {
        constexpr std::array<char, 6> blanks{ ' ', '\n', '\t', '\f', '\v', '\r' };
        return is_in_range(c, blanks.begin(), blanks.end());
    }

    constexpr auto is_operator_char(char c) noexcept {
        constexpr std::array<char, 5> ops{ '+', '-', '*', '/', '=' };
        return is_in_range(c, ops.begin(), ops.end());
    }

    constexpr auto is_digit(char c) noexcept {
        return c >= '0' && c <= '9';
    }
    constexpr auto is_id_start(lexer::char_t c) noexcept
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
                c == '_';
    }
    constexpr auto is_id_char(lexer::char_t c) noexcept
    {
        return is_id_start(c) || is_digit(c);
    }

    constexpr auto is_paren_open(char c) noexcept {
        return c == '(';
    }
    constexpr auto is_paren_close(char c) noexcept {
        return c == ')';
    }
    constexpr auto is_paren(char c) noexcept {
        return is_paren_open(c) || is_paren_close(c);
    }
    constexpr auto is_semi(lexer::char_t c) noexcept
    {
        return c == ';';
    }

    constexpr auto is_separator(char c) noexcept {
        return
            is_paren(c)     ||
            is_semi(c)      ||
            is_blank(c)     ||
            is_operator_char(c);
    }

    static auto lookup_keyword(token::value_type name) noexcept
    {
        using namespace std::literals;
        using kw_map = std::unordered_map<token::value_type, TokenType>;
        static kw_map keywords
        {
            { "var"sv, TokenType::KwVar }
        };

        auto found = keywords.find(name);
        return found != keywords.end() ? found->second : TokenType::Identifier;
    }

    lexer::lexer() noexcept :
        m_start{ m_buffer.end() },
        m_end{ m_buffer.end() }
    {}

    void lexer::operator()(value_type input) noexcept {
        m_buffer = input;
        m_start = m_buffer.begin();
        m_end = m_start;
        m_preview.reset();
        skip_spaces();
    }

    token lexer::next() noexcept {
        token t = peek();
        m_preview.reset();
        return t;
    }

    const token& lexer::peek() noexcept {
        if (m_preview)
            return *m_preview;

        if (!good())
            return consume(TokenType::Eof);

        const char next = peek_char();

        if (is_id_start(next))
            return identifier();
        if (is_digit(next))
            return number();
        if (is_operator_char(next))
            return op();
        if (is_separator(next))
            return punctuation();

        return consume(TokenType::Error);
    }

    bool lexer::good() const noexcept {
        return m_end != m_buffer.end();
    }

    lexer::char_t lexer::peek_char() const noexcept {
        return good() ? *m_end : char_t{};
    }

    void lexer::advance() noexcept {
        if (good())
            ++m_end;
    }

    lexer::value_type lexer::read_str() const noexcept {
        return { m_start, m_end };
    }

    void lexer::skip_spaces() noexcept {
        while (good() && is_blank(peek_char()))
            advance();
        m_start = m_end;
    }

    const token& lexer::consume(TokenType type) noexcept {
        if (type == TokenType::Error)
        {
            while (good() && !is_separator(peek_char()))
                advance();
            m_start = m_end;
        }

        auto token_str = read_str();
        if (type == TokenType::Identifier)
            type = lookup_keyword(token_str);

        auto val = (type != TokenType::Eof) ? read_str() : value_type{};
        skip_spaces();
        m_preview.emplace(val, type);
        return *m_preview;
    }

    const token& lexer::punctuation() noexcept {
        const char next = peek_char();
        advance();
        TokenType res = TokenType::Error;
        switch (next) {
            case '(': res = TokenType::ParenOpen;   break;
            case ')': res = TokenType::ParenClose;  break;
            case ';': res = TokenType::Semicolon;   break;
        }
        return consume(res);
    }

    const token& lexer::op() noexcept {
        const char next = peek_char();
        advance();
        TokenType res = TokenType::Error;
        switch (next) {
            case '+': res = TokenType::Plus;        break;
            case '-': res = TokenType::Minus;       break;
            case '*': res = TokenType::Asterisk;    break;
            case '/': res = TokenType::Slash;       break;
            case '=': res = TokenType::EqSign;      break;
        }
        return consume(res);
    }

    const token& lexer::number() noexcept
    {
        auto res = TokenType::Number;
        while (good())
        {
            const auto next = peek_char();
            if (is_separator(next))
                break;

            advance();
            if (is_digit(next))
            {
                continue;
            }

            res = TokenType::Error;
        }

        return consume(res);
    }

    const token& lexer::identifier() noexcept
    {
        auto res = TokenType::Identifier;
        while (good())
        {
            const auto next = peek_char();
            if (is_separator(next))
                break;

            advance();
            if (is_id_char(next))
            {
                continue;
            }

            res = TokenType::Error;
        }

        return consume(res);
    }
}
