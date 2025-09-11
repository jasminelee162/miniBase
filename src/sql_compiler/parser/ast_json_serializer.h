#pragma once

#include "ast.h"
#include "../../util/json.hpp"

using json = nlohmann::json;

namespace ASTJson {
    json toJson(const Statement* stmt);
}
