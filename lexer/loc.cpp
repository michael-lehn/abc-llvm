#include "loc.hpp"

namespace lexer {

std::ostream &
operator<<(std::ostream &out, Loc::Pos pos)
{
    out << pos.line << "." << pos.col;
    return out;
}

std::ostream &
operator<<(std::ostream &out, Loc loc)
{
    out << loc.path << ":" << loc.from << "-" << loc.to;
    return out;
}

} // namespace lexer
