#ifndef LEXER_READER_HPP
#define LEXER_READER_HPP

#include <fstream>
#include <iostream>
#include <memory>

#include "loc.hpp"

namespace lexer {

struct ReaderInfo {
    int			ch;
    UStr		path;
    Loc::Pos		start;
    Loc::Pos		pos;	
    std::string		val;
    std::ifstream	infile;
    std::istream	*in;

    ReaderInfo();
    ReaderInfo(const char *path);

    bool eof() const;
    bool valid() const;
    void resetStart();
};

extern std::unique_ptr<ReaderInfo> reader;

// if path is nullptr read from stdin
bool openInputfile(const char *path, bool search = false);
void addSearchPath(const char *path);

// read next character and update reader
char nextCh();

} // namespace lexer


#endif // LEXER_READER_HPP