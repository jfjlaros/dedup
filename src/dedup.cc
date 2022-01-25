#include <string>

#include "../lib/commandIO/src/commandIO.h"

#include "../../src/trie.tcc"
#include "../lib/ngs/ngs.h"

using std::ios;
using std::ofstream;
using std::string;

struct Cluster {
  size_t id;
  bool visited = false;

  Cluster(size_t id) { this->id = id; }
};

/*!
 * Leaf structure for neighbour finding.
 */
struct NLeaf : Leaf {
  vector<size_t> lines;
  vector<NLeaf*> neighbours;
  Cluster* cluster = NULL;

  ~NLeaf(void) { if (cluster) { delete cluster; } };
};


/*!
 * Traverse neighbours to assign cluster IDs.
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void assignCluster(NLeaf* leaf, Cluster* cluster) {
  leaf->cluster = cluster;
  for (NLeaf* neighbour: leaf->neighbours) {
    if (!neighbour->cluster) {
      assignCluster(neighbour, cluster);
    }
  }
}

/*!
 * Write a task start message to a log.
 *
 * \param log Log file.
 * \param message Message.
 *
 * \return Task start time.
 */
time_t startMessage(ofstream& log, char const message[]) {
  log << message << "... ";
  log.flush();

  return time(NULL);
}

/*!
 * Write a task end message to a log.
 *
 * \param log Log file.
 * \param start Task start time.
 */
void endMessage(ofstream& log, time_t start) {
  unsigned int seconds = (unsigned int)difftime(time(NULL), start);
  log << "done. (" << seconds / 60 << 'm' << seconds % 60 << "s)\n";
  log.flush();
}

/*!
 * Determine duplicates.
 *
 * \param read1 FastQ file for read 1.
 * \param read2 FastQ file for read 2.
 * \param umi FastQ file for the UMI.
 * \param length Read length.
 * \param distance Maximum hamming distance between reads.
 * \param outputName Output file.
 * \param logName Log file.
 */
void dedup(
    string read1, string read2, string umi, size_t length, size_t distance,
    string outputName, string logName) {
  Trie<4, NLeaf> trie;

  ofstream log(logName.c_str(), ios::out | ios::binary);
  time_t start = startMessage(log, "Reading data");
  size_t total = 0;
  size_t line = 0;
  for (Word word: readFiles(read1, read2, umi, length)) {
    if (!word.filtered) {
      NLeaf* leaf = trie.add(word.data);
      leaf->lines.push_back(line++);
    }
    total++;
  }
  endMessage(log, start);

  start = startMessage(log, "Calculating neighbours");
  size_t nonDuplicates = 0;
  for (Result<NLeaf> walkResult: trie.walk()) {
    for (Result<NLeaf> hammingResult: trie.hamming(
        walkResult.path, distance)) {
      if (walkResult.leaf != hammingResult.leaf) {
        walkResult.leaf->neighbours.push_back(hammingResult.leaf);
        hammingResult.leaf->neighbours.push_back(walkResult.leaf);
      }
    }
    nonDuplicates++;
  }
  endMessage(log, start);

  start = startMessage(log, "Calculating clusters");
  size_t id = 0;
  for (Result<NLeaf> result: trie.walk()) {
    if (!result.leaf->cluster) {
      Cluster* cluster = new Cluster(id++);
      assignCluster(result.leaf, cluster);
    }
  }
  endMessage(log, start);

/*
  start = startMessage(log, "Writing results");
  ofstream output(outputName.c_str(), ios::out | ios::binary);
  for (Result<NLeaf> result: trie.walk()) {
    for (size_t line: result.leaf->lines) {
      output << line << ' ' << result.leaf->cluster->id << '\n';
    }
  }
  output.close();
  endMessage(log, start);
  */

  /*
  */
  start = startMessage(log, "Writing results");
  for (Word word: readFiles(read1, read2, umi, length)) {
    if (!word.filtered) {
      Node<4, NLeaf>* node = trie.find(word.data);
      if (!node->leaf->cluster->visited) {
        // Emit reads.
        log << node->leaf->cluster->id << '\n';
        node->leaf->cluster->visited = true;
      }
    }
  }
  endMessage(log, start);

  log
    << "\nRead " << line << " out of " << total << " lines of length "
      << length << " (" << (float)(total - line) / total << "% discarded).\n"
    << "Left after removing perfect duplicates: " << nonDuplicates << " ("
      << 100 * (float)nonDuplicates / line << "%).\n"
    << "Left after removing nonperfect duplicates (distance " << distance
      << "): " << id << " (" << 100 * (float)(id) / line << "%).\n";

  log.close();
}


/*
 * Argument parsing.
 */
int main(int argc, char* argv[]) {
  CliIO io(argc, argv);

  interface(
    io,
    dedup, argv[0], "Deduplicate a dataset.", 
      param("read1", "FastQ file for read 1"),
      param("read2", "FastQ file for read 2"),
      param("umi", "FastQ file for the UMI"),
      param("length", "word length"),
      param("-d", 1, "distance"),
      param("-o", "/dev/stdout", "output file name"),
      param("-l", "/dev/stderr", "log file name"));

  return 0;
}
