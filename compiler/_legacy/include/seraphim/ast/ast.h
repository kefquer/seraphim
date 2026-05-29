#pragma once
#include "seraphim/ast/base.h"
#include "seraphim/ast/expr.h"
#include "seraphim/ast/decl.h"

namespace seraphim::ast
{
    template <typename T>
    concept ast_node = std::derived_from<T, node>;
}
