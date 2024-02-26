#ifndef READER_HPP
#define READER_HPP

#include <fstream>
#include <iostream>

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
bool openInputfile(const char *path);
bool addSearchPath(const char *path);

// read next character and update reader
char nextCh();

} // namespace lexer


#endif // READER_HPP
