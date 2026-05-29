#include "seraphim/ast/builder.h"
#include "seraphim/ast/decl.h"
#include "seraphim/ast/expr.h"
#include "seraphim/parser.h"
#include "seraphim/lexer.h"
#include "seraphim/token.h"
#include "seraphim/ast/visitor.h"

#include <cstddef>
#include <cstdio>
#include <string_view>
#include <iostream>
#include <print>

class ast_printer :
    public seraphim::ast::visitor<ast_printer>
{
    public:
        using base = seraphim::ast::visitor<ast_printer>;
        using child_count = std::size_t;
        using child_tracker = std::vector<child_count>;
        using size_type = child_tracker::size_type;

    public:
        ast_printer() = default;
        ~ast_printer() = default;

        ast_printer(const ast_printer&) = delete;
        ast_printer& operator = (const ast_printer&) = delete;
        ast_printer(ast_printer&&) = delete;
        ast_printer& operator = (ast_printer&&) = delete;

        void operator()(const seraphim::ast::node* node)
        {
            m_indetations.clear();
            base::operator()(node);
        }

    public:
        // Ловушка №1: Ссылка с const
        bool preview(const seraphim::ast::node& node) {
            std::cout << "[FOUND] Визитор вызвал preview(const node&)\n" << std::flush;
            return true;
        }

        // Ловушка №2: Ссылка без const
        bool preview(seraphim::ast::node& node) {
            std::cout << "[FOUND] Визитор вызвал preview(node&)\n" << std::flush;
            return true;
        }

        // Ловушка №3: Указатель
        bool preview(const seraphim::ast::node* node) {
            std::cout << "[FOUND] Визитор вызвал preview(const node*)\n" << std::flush;
            return true;
        }

        bool preview(const seraphim::ast::lit_expr& expr)
        {
            indent();
            std::cout << "literal '" << expr.value().value() << "'\n";
            return true;
        }

        bool preview(const seraphim::ast::paren_expr&)
        {
            indent();
            std::cout << "paren\n";
            m_indetations.push_back(1u);
            return true;
        }

        bool preview(const seraphim::ast::unary_expr& expr)
        {
            indent();
            std::cout << "unary '";
            switch (expr.op())
            {
                case seraphim::ast::operation::UnaryPos: std::cout << '+'; break;
                case seraphim::ast::operation::UnaryNeg: std::cout << '-'; break;

                default: std::cout << "unknown"; break;
            }

            std::cout << "'\n";
            m_indetations.push_back(1u);
            return true;
        }

        bool preview(const seraphim::ast::binary_expr& expr)
        {
            indent();
            std::cout << "binary '";
            switch (expr.op())
            {
                case seraphim::ast::operation::Addition:        std::cout << '+'; break;
                case seraphim::ast::operation::Substraction:    std::cout << '-'; break;
                case seraphim::ast::operation::Multiplication:  std::cout << '*'; break;
                case seraphim::ast::operation::Division:        std::cout << '/'; break;

                default: std::cout << "unknown"; break;
            }

            std::cout << "'\n";
            m_indetations.push_back(2u);
            return true;
        }

        bool preview(const seraphim::ast::list& list)
        {
            indent();
            std::cout << "list\n";
            m_indetations.push_back(list.children().size());
            return true;
        }

        bool preview(const seraphim::ast::id_expr& expr)
        {
            using namespace std::literals;
            indent();
            std::cout << "variable ref '" << expr.name().value() << "' [";
            std::print(std::cout, "{:X}"sv, reinterpret_cast<std::size_t>(&expr.declaration()));
            std::cout << "]\n";
            return true;
        }

        bool preview(const seraphim::ast::var_decl& decl)
        {
            using namespace std::literals;
            indent();
            std::cout << "variable '" << decl.name().value() << "' [";
            std::print(std::cout, "{:X}"sv, reinterpret_cast<std::size_t>(&decl));
            std::cout << "]\n";
            m_indetations.push_back(1u);
            return true;
        }

    private:
        void indent()
        {
            while (!m_indetations.empty())
            {
                if (m_indetations.back())
                    break;

                m_indetations.pop_back();
            }

            if (m_indetations.empty())
                return;

            auto&& last = m_indetations.back();
            --last;
            for (auto&& cur : m_indetations)
            {
                if (&cur != &last)
                {
                    print_parent(cur);
                    continue;
                }

                print_child(last);
            }
        }

    void print_parent(child_count count)
    {
        using namespace std::literals;
        static constexpr auto childIndent = "| "sv;
        static constexpr auto blackIndent = "  "sv;

        if (!count)
            std::cout << blackIndent;
        else
            std::cout << childIndent;
    }

    void print_child(child_count count)
    {
        using namespace std::literals;
        static constexpr auto prefix = "|-"sv;
        static constexpr auto lastPrefix = "`-"sv;

        if (!count)
            std::cout << lastPrefix;
        else
            std::cout << prefix;
    }

    private:
        child_tracker m_indetations;
};

void echo(std::string_view input)
{
    std::cout << "input: " << input << "\n\n";
}

void test_parser(std::string_view input)
{
    echo(input);
    seraphim::ast::builder builder;
    seraphim::parser parser{ builder };
    std::cout << "[DEBUG] Отправляем в парсер...\n" << std::flush;
    parser(input);
    std::cout << "[DEBUG] Парсер отработал успешно!\n" << std::flush;
    ast_printer{}(builder.root());
    std::cout << "\n[DEBUG] Принтер отработал успешно!\n" << std::flush;

    std::cout << std::flush;
    builder.clear_state();
}

void test_lex(std::string_view input)
{
    echo(input);
    seraphim::lexer lexer;
    lexer(input);
    for(;;)
    {
        auto t = lexer.next();
        switch (t.what())
        {
            case seraphim::TokenType::Eof:
                std::cout << "eof\n"; return;
            case seraphim::TokenType::Number:
                std::cout << "number"; break;

            case seraphim::TokenType::Plus:
            case seraphim::TokenType::Minus:
            case seraphim::TokenType::Asterisk:
            case seraphim::TokenType::Slash:
                std::cout << "operator"; break;

            case seraphim::TokenType::ParenOpen:
            case seraphim::TokenType::ParenClose:
                std::cout << "paren"; break;

            default:
                std::cout << "error"; break;
        }

        std::cout << ": '" << t.value() << "'\n";
    }
}

int main()
{
    test_parser("var x = 10 + 3; var y = x + (10*(2+(3/2)));");
    return 0;
}
