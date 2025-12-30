#include <cassert>
#include <unordered_set>
#include <vector>

#include "macro.hpp"

template <> struct std::hash<abc::lexer::Token>
{
	std::size_t
	operator()(const abc::lexer::Token &token) const noexcept
	{
	    return std::hash<const char *>{}(token.val.c_str());
	}
};

namespace abc {
namespace lexer {
namespace macro {

static std::unordered_map<Token, std::vector<Token>> define;
static bool insideIfdef;
static bool ignoreToken_;
static std::vector<Token> token;

void
init()
{
    define.clear();
    insideIfdef = ignoreToken_ = false;
}

bool
ignoreToken()
{
    return ignoreToken_;
}

bool
ifndefDirective(Token identifier)
{
    assert(identifier.kind == TokenKind::IDENTIFIER);
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
defineDirective(Token identifier, std::vector<Token> &&replacement)
{
    assert(identifier.kind == TokenKind::IDENTIFIER);
    bool ok = true;
    if (!ignoreToken()) {
	if (!define.contains(identifier)) {
	    define[identifier] = std::move(replacement);
	} else {
	    ok = false;
	}
    }
    return ok;
}

static bool
expandMacro_(Token identifier, std::unordered_set<Token> expanded = {})
{
    if (expanded.contains(identifier) || !define.contains(identifier)) {
	return false;
    }
    auto replacement = define.at(identifier);
    expanded.insert(identifier);
    while (!replacement.empty()) {
	auto t = replacement.back();
	t.loc = identifier.loc;
	replacement.pop_back();

	if (!expandMacro_(t, expanded)) {
	    token.push_back(t);
	}
    }
    return true;
}

bool
expandMacro(Token identifier)
{
    if (!define.contains(identifier)) {
	return false;
    }

    expandMacro_(identifier);
    return true;
}

bool
hasToken()
{
    return token.size();
}

Token
getToken()
{
    assert(hasToken());
    auto t = token.back();
    token.pop_back();
    return t;
}

} // namespace macro
} // namespace lexer
} // namespace abc
