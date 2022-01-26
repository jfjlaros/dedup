#include <iostream>
#include <map>

#include "ngs.h"

using std::cout;
using std::ios;
using std::map;

map<char, uint8_t> nuc = {{'A', 0}, {'C', 1}, {'G', 2}, {'T', 3}};

/*
 * Read vector.
 */
struct _ReadVector {
  vector<Read*> reads;
  bool eof = false;

  void free(void);
};


void _ReadVector::free(void) {
  for (Read* read: reads) {
    delete read;
  }
}


/*
 * Read one FastQ record from multiple files.
 *
 * \param readers FastQ readers.
 *
 * \return FastQ records plus EOF status.
 */
_ReadVector _readFastq(vector<FastqReader*>& readers) {
  _ReadVector readVector;
  for (FastqReader* reader: readers) {
    Read* read = reader->read();
    if (!read) {
      readVector.eof = true;
    }
    readVector.reads.push_back(read);
  }
  return readVector;
}

/*!
 * Loop over all reads in multiple FastQ files.
 *
 * \param files FastQ file names.
 *
 * \return All reads.
 */
generator<vector<Read*>> readFiles(vector<string>& files) {
  vector<FastqReader*> readers;
  for (string file: files) {
    FastqReader* reader = new FastqReader(file.c_str());
    readers.push_back(reader);
  }

  _ReadVector readVector = _readFastq(readers);
  while (!readVector.eof) {
    co_yield readVector.reads;
    readVector.free();
    readVector = _readFastq(readers);
  }
  // TODO: Extra readVector.free() here when files are not of equal length?

  for (FastqReader* reader: readers) {
    delete reader;
  }
}

/*!
 * Select `length` nucleotides from every read in `reads` to create a word.
 *
 * \param reads Reads.
 * \param length Read selection length.
 *
 * \return Word.
 */
Word makeWord(vector<Read*>& reads, size_t length) {
  Word word;
  for (Read* read: reads) {
    for (size_t i = 0; i < length; i++) {
      char nucleotide = (*read->mSeq)[i];
      if (nuc.contains(nucleotide)) {
        word.data.push_back(nuc[nucleotide]);
      }
      else {
        word.data.push_back(nuc['G']);
        word.filtered = true;
      }
    }
  }
  return word;
}

/*!
 * Print a word.
 *
 * \param word Word.
 */
void printWord(vector<uint8_t>& word) {
  for (uint8_t letter: word) {
    cout << ' ' << (int)letter;
  }
  cout << '\n';
}
