#include "cluster.h"
#include "leaf.h"


/*!
 * Constructor.
 */
Cluster::Cluster(size_t id) {
  this->id = id;
}


/*!
 * Assign leaf to cluster.
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void _assignLeaf(NLeaf* leaf, Cluster* cluster) {
  leaf->cluster = cluster;
  leaf->cluster->size += leaf->count;
}

/*
 * Update the counts for cluster.
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void _updateMaxCount(NLeaf* leaf, Cluster* cluster) {
  if (leaf->count > cluster->maxCount) {
    cluster->maxLeaf = leaf;
    cluster->maxCount = leaf->count;
  }
}

/*!
 * Traverse neighbours to assign cluster IDs
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void assignMaxCluster(NLeaf* leaf, Cluster* cluster) {
  _assignLeaf(leaf, cluster);
  _updateMaxCount(leaf, cluster);
  for (NLeaf* neighbour: leaf->neighbours) {
    if (!neighbour->cluster) {
      assignMaxCluster(neighbour, cluster);
    }
  }
}

/*!
 * Determine if a is at least 2b. This is used on the the count difference
 * between a leaf and its neighbour. If this is the case, neighbour will be
 * treated as a PCR-amplified error of leaf.
 */
bool _atLeastDouble(int a, int b) {
  return a > 2 * b - 1;
}

/*
 * The inverse of _atLeastDouble, used to determine if the neighbour is the
 * true sequence, and leaf a PCR error.
 */
bool _atMostHalf(int a, int b) {
  return _atLeastDouble(b, a);
}

/*!
 * Traverse neigbhours until a local maximum is reached.
 *
 * \param leaf Leaf node.
 */
NLeaf* max_neighbour(NLeaf* leaf){
  int i = 0;
  while (i < leaf->neighbours.size()) {
    NLeaf* neighbour = leaf->neighbours[i++];

    if (not neighbour->cluster and _atLeastDouble(neighbour->count, leaf->count)) {
      // Go to the neighbour and repeat.
      leaf = neighbour;
      i = 0;
    }
  }
  return leaf;
}

/*
 * Internal function to traverse neighbours to assign cluster ID
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void _assignDirectionalCluster(NLeaf* leaf, Cluster* cluster) {
  _assignLeaf(leaf, cluster);
  for (NLeaf* neighbour: leaf->neighbours) {
    // If we encounter a neighbour that is unassigned, at most half leaf's
    // size, we recursively add it to cluster.
    if (!neighbour->cluster and _atMostHalf(neighbour->count, leaf->count)) {
      // If the neighbour has less than half of the number of reads, the
      // neighbour belongs to the current cluster.
      _assignDirectionalCluster(neighbour, cluster);
    }
  }
}

/*!
 * Traverse neighbours to assign cluster IDs, using the directional method
 *
 * Also updates the max_count for the cluster after determining a maximum
 * neighbour
 *
 * \param leaf Leaf node.
 * \param cluster Cluster.
 */
void assignDirectionalCluster(NLeaf* leaf, Cluster* cluster){
  NLeaf* node = max_neighbour(leaf);
  // Update the maxcount for the cluster using the max node (only once)
  _updateMaxCount(node, cluster);
  _assignDirectionalCluster(node, cluster);
}

/*!
 * Make a histogram of cluster sizes.
 *
 * \param clusters List of clusters.
 *
 * \return Histogram of cluster sizes.
 */
map<size_t, size_t> clusterStats(vector<Cluster*>& clusters) {
  map<size_t, size_t> counts;
  for (Cluster* cluster: clusters) {
    counts[cluster->size]++;
  }
  return counts;
}

/*!
 * Destroy a list of clusters.
 *
 * \param clusters List of clusters.
 */
void freeClusters(vector<Cluster*>& clusters) {
  for (Cluster* cluster: clusters) {
    delete cluster;
  }
}
