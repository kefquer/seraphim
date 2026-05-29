#include "seraphim/ast/decl.h"
#include "seraphim/ast/base.h"
#include "seraphim/token.h"

namespace seraphim::ast
{
    decl::decl(node_t t, const token& name) noexcept :
        node{ t },
        m_name{ name }
    { }

    const token& decl::name() const noexcept
    {
        return m_name;
    }

    var_decl::var_decl(const token& name, node& init) noexcept :
        decl{ node::VarDecl, name },
        m_init{ &init }
    {
    }

    const node& var_decl::initialiser() const noexcept
    {
        return *m_init;
    }
}
