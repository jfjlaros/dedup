#include <catch.hpp>
#include "../src/cluster.cc"

// Helper function to link nodes
void link(NLeaf* a, NLeaf* b){
    a->neighbours.push_back(b);
    b->neighbours.push_back(a);
}

TEST_CASE("Test if a is at least 2x b", "[cluster]"){
    REQUIRE(_atLeastDouble(1, 0));
    REQUIRE(_atLeastDouble(2, 1));
    REQUIRE(!_atLeastDouble(3,2));
}

TEST_CASE("Test if a is at most half of b", "[cluster]"){
    REQUIRE(_atMostHalf(0, 1));
    REQUIRE(_atMostHalf(1, 2));
    REQUIRE(!_atMostHalf(2, 3));
}

TEST_CASE("Test walking a node with no neighbours", "[cluster]"){

    // Create a node that is all alone
    NLeaf alone;
    //A leaf with no neighbours should return itself
    REQUIRE(max_neighbour(&alone) == &alone);
}

TEST_CASE("Test walking node whose neighbour is already assigned", "[cluster]"){
    // Create a more complex setup, of chained leafs
    NLeaf* leaf = new NLeaf();
    leaf->count = 1;

    //A neighbour that is already in a cluster should not be used
    NLeaf* assigned_neighbour = new NLeaf();
    Cluster* c = new Cluster(2);
    assigned_neighbour->cluster = c;
    assigned_neighbour->count = 2;

    link(leaf, assigned_neighbour);
    REQUIRE(max_neighbour(leaf) == leaf);

    delete leaf; delete assigned_neighbour; delete c;
}

TEST_CASE("Test walking a chain of nodes", "[cluster]"){
    //If there is a neighbour that is not assigned and it conforms to the 2x
    //requirement, it should be used
    NLeaf* leaf = new NLeaf();
    leaf->count = 1;

    NLeaf* free_neighbour = new NLeaf();
    free_neighbour->count=2;

    link(leaf, free_neighbour);
    REQUIRE(max_neighbour(leaf) == free_neighbour);

    //Lets test a third, further neighbour
    NLeaf* third_neighbour = new NLeaf();
    third_neighbour->count=4;
    link(free_neighbour, third_neighbour);
    REQUIRE(max_neighbour(leaf) == third_neighbour);

    //Add one more neighbour, that is not high enough to add
    NLeaf* last_one = new NLeaf();
    last_one->count=7;
    link(third_neighbour, last_one);

    // Check that the last neighbour was not added, since it was not high
    // enough
    REQUIRE(max_neighbour(leaf) == third_neighbour);

    delete leaf; delete free_neighbour; delete third_neighbour; delete last_one;
}

TEST_CASE("Test assigning to cluster") {
    // Initialise the node counts
    NLeaf* node1 = new NLeaf(); // smallest node
    node1->count = 2;

    NLeaf* node2 = new NLeaf(); // intermediate node
    node2->count = 4;

    NLeaf* node3 = new NLeaf(); // largest reachable node by walking up in 2x steps
    node3->count = 8;

    NLeaf* node4 = new NLeaf(); // largest overall node, only reachable from node5
    node4->count = 10;

    NLeaf* node5 = new NLeaf(); // Only reachable from node5
    node5->count = 3;

    // Test the inialisation
    REQUIRE(node4->count == 10);

    // Link node1 and node2 together
    link(node1, node2);

    // Test that node1 and node2 are now linked
    REQUIRE(node1->neighbours.front() == node2);
    REQUIRE(node2->neighbours.front() == node1);

    // Link the rest of the nodes
    link(node2, node3);
    link(node3, node4);
    link(node4, node5);

    // Assume we start with node 1, and want to assign them to cluster 1
    Cluster* cluster1 = new Cluster(1);

    // Next, we assign all nodes
    assignDirectionalCluster(node1, cluster1);

    // Then, the first three nodes should be assigned to cluster 1
    REQUIRE(node1->cluster->id == 1);
    REQUIRE(node2->cluster->id == 1);
    REQUIRE(node3->cluster->id == 1);

    // The last two node should not be assigned
    REQUIRE(!node4->cluster);
    REQUIRE(!node5->cluster);

    // Now we assign node4 to cluster2
    Cluster* cluster2 = new Cluster(2);
    assignDirectionalCluster(node4, cluster2);
    REQUIRE(node4->cluster->id == 2);
    REQUIRE(node5->cluster->id == 2);

    // Check that the cluster size is correct
    REQUIRE(cluster1->size == 14);
    REQUIRE(cluster2->size == 13);

    // Check that the maxleaf is correct
    REQUIRE(cluster1->maxLeaf == node3);
    REQUIRE(cluster2->maxLeaf == node4);

    // Check that the maxCount is correct
    REQUIRE(cluster1->maxCount == 8);
    REQUIRE(cluster2->maxCount == 10);

    // Clean up
    delete cluster1; delete cluster2;
    delete node1; delete node2; delete node3; delete node4; delete node5;
}
