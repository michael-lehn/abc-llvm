#include <cassert>
#include <cstdlib>

#include "lexer.hpp"
#include "reader.hpp"
#include "ustr.hpp"

namespace lexer {

ReaderInfo::ReaderInfo()
    : ch{0}, path{UStr::create("<stdin>")}, start{1, 1}, pos{1, 1}, val{}
    , infile{} , in{&std::cin}
{
}

ReaderInfo::ReaderInfo(const char *path)
    : ch{0}, path{UStr::create(path)}, start{0, 1}, pos{0, 1}, val{}
    , infile{path}, in{nullptr}
{
    if (infile.is_open()) {
	in = &infile;
    } else {
	assert(0 && "can not open file");
	std::exit(1);
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

std::unique_ptr<ReaderInfo> reader;
static std::vector<std::unique_ptr<ReaderInfo>> openReader;

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
    return reader->ch;
}

// if path is nullptr read from stdin
bool
openInputfile(const char *path)
{
    if (reader) {
	assert(reader->valid());
	openReader.push_back(std::move(reader));
    }
    reader = path
	? std::make_unique<ReaderInfo>(path)
	: std::make_unique<ReaderInfo>();
    return reader->valid();
}

bool
addSearchPath(const char *path)
{
    assert(0 && "not implemented");
}

} // namespace lexer
