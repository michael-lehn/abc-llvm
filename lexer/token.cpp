#include "token.hpp"
#include "reader.hpp"

namespace lexer {

std::ostream &
operator<<(std::ostream &out, const Token &token)
{
    out << token.loc << ": "
	<< TokenKindCStr(token.kind) << " "
	<< "'" << token.val << "' "
	<< "'" << token.processedVal << "' "
	<< "('" << token.kind << "')";
    return out;
}

} // namespace lexer
