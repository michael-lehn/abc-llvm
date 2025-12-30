#ifndef LEXER_READER_HPP
#define LEXER_READER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "loc.hpp"

namespace abc {
namespace lexer {

struct ReaderInfo
{
	int ch;
	UStr path;
	Loc::Pos start;
	Loc::Pos pos;
	std::string val;
	std::ifstream infile;
	std::istream *in;

	ReaderInfo();
	ReaderInfo(const char *path);

	bool eof() const;
	bool valid() const;
	void resetStart();
};

extern std::unique_ptr<ReaderInfo> reader;

std::filesystem::path searchFile(std::filesystem::path path);

// if path is empty read from stdin
bool openInputfile(std::filesystem::path path);
void addSearchPath(std::filesystem::path path);
const std::vector<std::filesystem::path> &getSearchPath();

// read next character and update reader
char nextCh();

} // namespace lexer
} // namespace abc

#endif // LEXER_READER_HPP
