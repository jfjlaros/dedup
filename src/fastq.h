#pragma once

#include <string>
#include <vector>

#include "../lib/trie/lib/CPP20Coroutines/include/generator.hpp"
#include "../lib/fastp/src/fastqreader.h"

using std::string;
using std::vector;

/*!
 * Word.
 */
struct Word {
  vector<uint8_t> data;
  bool filtered = false;
};

generator<vector<Read*>> readFiles(vector<string>);
Word makeWord(vector<Read*>&, size_t);
void printWord(vector<uint8_t>&);
string addDir(char const[], string);
string makeFileName(string, string, string);
vector<string> makeFileNames(vector<string>, string, string);
