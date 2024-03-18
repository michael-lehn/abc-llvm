#include <cassert>
#include <cstdint>
#include <cstdlib>

#include "lexer.hpp"
#include "reader.hpp"
#include "util/ustr.hpp"

namespace abc { namespace lexer {

ReaderInfo::ReaderInfo()
    : ch{0}, path{UStr::create("<stdin>")}, val{} , infile{} , in{&std::cin}
{
}

ReaderInfo::ReaderInfo(const char *path)
    : ch{0}, path{UStr::create(path)}, val{} , infile{path}, in{nullptr}
{
    if (infile.is_open()) {
	in = &infile;
    }
}

bool
ReaderInfo::valid() const
{
    return in;
}

bool
ReaderInfo::eof() const
{
    return in->eof();
}

void
ReaderInfo::resetStart()
{
    start = pos;
    val = "";
}

//------------------------------------------------------------------------------

std::unique_ptr<ReaderInfo> reader;
static std::vector<std::unique_ptr<ReaderInfo>> openReader;
static std::vector<std::filesystem::path> searchPath;

// read next character and update reader
char
nextCh()
{
    assert(reader);

    constexpr std::size_t tabStop = 8;

    if (reader->ch) {
	reader->val += reader->ch;
	if (reader->ch == '\t') {
	    reader->pos.col += tabStop - reader->pos.col % tabStop;
	} else if (reader->ch == '\n') {
	    ++reader->pos.line;
	    reader->pos.col = 1;
	} else {
	    ++reader->pos.col;
	}
    }
    reader->ch = reader->in->get();
    if (reader->eof() && !openReader.empty()) {
	reader = std::move(openReader.back());
	openReader.pop_back();
	return nextCh();
    } else {
	return reader->ch;
    }
}

std::filesystem::path
searchFile(std::filesystem::path path)
{
    for (auto sp: searchPath) {
	sp /= path;
	std::ifstream f(sp.c_str());
	if (f.good()) {
	    return sp;
	}
    }
    return "";
}

// if path is nullptr read from stdin
bool
openInputfile(std::filesystem::path path)
{
    if (reader) {
	assert(reader->valid());
	openReader.push_back(std::move(reader));
    }
    reader = !path.empty()
	? std::make_unique<ReaderInfo>(path.c_str())
	: std::make_unique<ReaderInfo>();
    if (!reader->valid()) {
	return false;
    } else {
	nextCh();
	return true;
    }
}

void
addSearchPath(std::filesystem::path path)
{
    searchPath.push_back(path);
}

} } // namespace lexer, abc
