#include "seraphim/lexer.h"
#include "seraphim/token.h"
#include <cctype>
#include <string_view>
#include <unordered_map>

namespace Lexer {

    static bool isWhitespace(char c) noexcept {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    static bool isIdentifierStart(char c) noexcept {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    static bool isIdentifierPart(char c) noexcept {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    // Tab main word
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        {"absolute",    TokenType::KwAbsolute},
        {"freed",       TokenType::KwFreed},
        {"visible",     TokenType::KwVisible},
        {"invisible",   TokenType::KwInvisible},
        {"breakcore",   TokenType::KwBreakcore},
        {"done",        TokenType::KwDone},
        {"int",         TokenType::KwInt},
        {"bool",        TokenType::KwBool},
        {"string",      TokenType::KwString},
        {"class",       TokenType::KwClass},
        {"if",          TokenType::KwIf},
        {"else",        TokenType::KwElse},
        {"while",       TokenType::KwWhile},
        {"return",          TokenType::KwReturn},
        {"true",        TokenType::BoolLiteral},
        {"false",       TokenType::BoolLiteral},
    };

    TokenType Lexer::resolveKeyword(StringT word) noexcept {
        auto it = keywords.find(word);
        if (it != keywords.end())
            return it->second;
        return TokenType::Identifier;
    }

    // Public Methods
    void Lexer::reset(StringT source) noexcept {
        m_source    = source;
        m_current   = m_source.begin();
        m_start     = m_current;
        m_peek.reset();
        skip_WhitespaceAndComments();
    }

    Token Lexer::next() noexcept {
        if (m_peek) {
            Token t = std::move(*m_peek);
            m_peek.reset();
            return t;
        }
        peek();
        Token t = std::move(*m_peek);
        m_peek.reset();
        return t;
    }

    const Token& Lexer::peek() noexcept {
        if (m_peek)
            return *m_peek;

        skip_WhitespaceAndComments();

        if (eof()) {
            m_start = m_current;
            m_peek = makeToken(TokenType::Eof);
            return *m_peek;
        }

        m_start = m_current;
        char c = currentChar();

        if (std::isdigit(static_cast<unsigned char>(c))) {
            m_peek = read_Number();
        }
        else if (c == '"') {
            m_peek = read_String();
        }
        else if (c == '!') {
            m_peek = read_IncludeDirective();
        }
        else if (isIdentifierStart(c)) {
            m_peek = read_IdentifierOrKeyword();
        }
        else {
            m_peek = read_OperatorOrPunctuation();
        }

        return *m_peek;
    }

    // Private Methods
    bool Lexer::eof() const noexcept {
        return m_current == m_source.end();
    }

    Lexer::CharT Lexer::currentChar() const noexcept {
        if (eof()) return '\0';
        return *m_current;
    }

    void Lexer::advance() noexcept {
        if (!eof()) ++m_current;
    }

    void Lexer::skip_WhitespaceAndComments() noexcept {
        while (!eof()) {
            char c = currentChar();

            if (isWhitespace(c)) {
                advance();
                continue;
            }

            if (c == '/' && !eof()) {
                auto next = m_current;
                ++next;
                if (next != m_source.end() && *next == '/') {
                    while (!eof() && currentChar() != '\n')
                        advance();
                    continue;
                }
                break;
            }

            if (c == '/' && !eof()) {
                auto next = m_current;
                ++next;
                if (next != m_source.end() && *next == '*') {
                    advance();
                    advance();

                    while (!eof()) {
                        if (currentChar() == '*' && !eof()) {
                            advance();
                            if (!eof() && currentChar() == '/') {
                                advance();
                                break;
                            }
                        } else {
                            advance();
                        }
                    }
                    continue;
                }
                break;
            }

            break;
        }
    }

    Token Lexer::read_Number() noexcept {
        while (!eof() && std::isdigit(static_cast<unsigned char>(currentChar())))
            advance();
        return makeToken(TokenType::Number);
    }

    Token Lexer::read_String() noexcept {
        advance();
        while (!eof() && currentChar() != '"') {
            if (currentChar() == '\\') {
                advance();
                if (!eof()) advance();
            } else {
                advance();
            }
        }
        if (!eof()) advance();

        if (!eof() && currentChar() == 'L') {
            advance();
        }
        return makeToken(TokenType::StringLiteral);
    }

    Token Lexer::read_IdentifierOrKeyword() noexcept {
        while (!eof() && isIdentifierPart(currentChar()))
            advance();
        StringT word(m_start, m_current);
        TokenType type = resolveKeyword(word);
        return Token(word, type);
    }

    Token Lexer::read_OperatorOrPunctuation() noexcept {
        char c = currentChar();
        advance();

        switch (c) {
            case '+': return makeToken(TokenType::Plus);
            case '-':
                if (!eof() && currentChar() == '>') {
                    advance();
                    return makeToken(TokenType::Arrow);
                }
                return makeToken(TokenType::Minus);
            case '*': return makeToken(TokenType::Asterisk);
            case '/': return makeToken(TokenType::Slash);
            case '%': return makeToken(TokenType::Percent);
            case '=':
                if (!eof() && currentChar() == '=') {
                    advance();
                    return makeToken(TokenType::Equal);
                }
                return makeToken(TokenType::Assign);
            case '!':
                if (!eof() && currentChar() == '=') {
                    advance();
                    return makeToken(TokenType::NotEqual);
                }
                return makeToken(TokenType::LogicalNot);
            case '<':
                if (!eof() && currentChar() == '=') {
                    advance();
                    return makeToken(TokenType::LessEqual);
                }
                return makeToken(TokenType::LessThan);
            case '>':
                if (!eof() && currentChar() == '=') {
                    advance();
                    return makeToken(TokenType::GreaterEqual);
                }
                return makeToken(TokenType::GreaterThan);
            case '&':
                if (!eof() && currentChar() == '&') {
                    advance();
                    return makeToken(TokenType::LogicalAnd);
                }
                break;
            case '|':
                if (!eof() && currentChar() == '|') {
                    advance();
                    return makeToken(TokenType::LogicalOr);
                }
                break;
            case ':':
                if (!eof() && currentChar() == ':') {
                    advance();
                    return makeToken(TokenType::DoubleColon);
                }
                return makeToken(TokenType::Colon);
            case '(': return makeToken(TokenType::ParenOpen);
            case ')': return makeToken(TokenType::ParenClose);
            case '{': return makeToken(TokenType::BraceOpen);
            case '}': return makeToken(TokenType::BraceClose);
            case ';': return makeToken(TokenType::Semicolon);
            case ',': return makeToken(TokenType::Comma);
            default: break;
        }

        return makeToken(TokenType::Error);
    }

    Token Lexer::read_IncludeDirective() noexcept {
        advance();
        while (!eof() && isIdentifierPart(currentChar()))
            advance();

        skip_WhitespaceAndComments();
        if (!eof() && currentChar() == '[') {
            advance();
            while (!eof() && currentChar() != ']') {
                advance();
            }
            if (!eof()) advance();
        }
        return makeToken(TokenType::KwInclude);
    }

    Token Lexer::makeToken(TokenType type) const noexcept {
        return Token(StringT(m_start, m_current), type);
    }
}
