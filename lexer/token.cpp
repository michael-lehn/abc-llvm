#include "token.hpp"
#include "reader.hpp"

namespace abc { namespace lexer {

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

bool
operator==(const abc::lexer::Token &x, const abc::lexer::Token &y)
{
    return x.kind == y.kind && x.val == y.val;
}


} } // namespace lexer, abc
