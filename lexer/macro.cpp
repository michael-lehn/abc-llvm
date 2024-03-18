#include "macro.hpp"

namespace abc { namespace lexer { namespace macro {

static std::unordered_map<UStr, UStr> define;
static bool insideIfdef;
static bool ignoreToken_;


bool
ignoreToken()
{
    return ignoreToken_;
}

bool
ifndefDirective(const UStr identifier)
{
    bool ok = !insideIfdef;
    insideIfdef = true;
    if (!define.contains(identifier)) {
	ignoreToken_ = true;
    }
    return ok;
}

void
endifDirective()
{
    insideIfdef = false;
    ignoreToken_ = false;
}

bool
defineDirective(const UStr identifier, const UStr replacement)
{
    bool ok = true;
    if (!ignoreToken()) {
	if (!define.contains(identifier)) {
	    define[identifier] = replacement;
	} else {
	    ok = false;
	}
    }
    return ok;
}

bool
expandMacro(UStr identifier, UStr &replacement)
{
    bool found = false;
    replacement = identifier;
    while (define.contains(replacement)) {
	found = true;
	replacement = define.at(replacement);
	if (identifier == replacement) {
	    break;
	}
    }
    return found;
}

} } } // namespace macro, lexer, abc
