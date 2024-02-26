#include <cassert>
#include <filesystem>
#include <cstdlib>

#include "lexer.hpp"
#include "reader.hpp"
#include "ustr.hpp"

namespace lexer {

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

// if path is nullptr read from stdin
bool
openInputfile(const char *path_, bool search)
{
    std::filesystem::path path;

    if (path_) {
	path = path_;

	bool found = false;
	if (search) {
	    for (auto sp: searchPath) {
		sp /= path;
		std::ifstream f(sp.c_str());
		if (f.good()) {
		    found = true;
		    path = sp;
		    break;
		}
	    }
	} else {
	    std::ifstream f(path.c_str());
	    if (f.good()) {
		found = true;
	    }
	}
	if (!found) {
	    return false;
	}
    }

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
addSearchPath(const char *path)
{
    searchPath.push_back(path);
}

} // namespace lexer
