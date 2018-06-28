// C/C++ includes
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <ctime>
#include <iostream>
#include <string>

// OSG includes
#include <osg/Geode>
#include <osg/Geometry>

#define BOOST_TEST_MODULE "KDTree_test"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_KDTree)

// node of k-d tree
struct KDnode
{
    osg::Vec3f v0, v1, v2;
    osg::Vec3f midpoint;
    osg::Vec3f normal;
    KDnode *left, *right;

    KDnode(osg::Vec3f k)
        : midpoint(k) {
            left = NULL;
            right = NULL;
        }

    KDnode( osg::Vec3f a, osg::Vec3f b, osg::Vec3f c, osg::Vec3f k, osg::Vec3f n)
        : v0(a), v1(b), v2(c), midpoint(k), normal(n) {
            left = NULL;
            right = NULL;
        }
};

// Calculate the distance between two points
inline double dist(KDnode *a, KDnode *b)
{
    return ((a->midpoint - b->midpoint).length2());
}

// Switch the values of node A and B
inline void swap(KDnode *a, KDnode *b)
{
    osg::Vec3f tmp = a->midpoint;
    a->midpoint = b->midpoint;
    b->midpoint = tmp;
}

// Find the median value of binary tree
KDnode *findMedian(KDnode *start, KDnode *end, int idx)
{
    if (end <= start)
        return NULL;
    if (end == start + 1)
        return start;

    KDnode *p, *store, *md = start + (end - start) / 2;
    double pivot;
    while (1)
    {
        pivot = md->midpoint[idx];

        swap(md, end - 1);
        for (store = p = start; p < end; p++)
        {
            if (p->midpoint[idx] < pivot)
            {
                if (p != store)
                    swap(p, store);
                store++;
            }
        }
        swap(store, end - 1);

        // Median has duplicate values */
        if (store->midpoint[idx] == md->midpoint[idx])
            return md;

        if (store > md)
            end = store;
        else
            start = store;
    }
}

// Build the k-d tree
KDnode *makeTree(KDnode *t, int len, int i, int dim)
{
    KDnode *n;

    if (!len)
        return 0;

    if ((n = findMedian(t, t + len, i)))
    {
        i = (i + 1) % dim;
        n->left = makeTree(t, n - t, i, dim);
        n->right = makeTree(n + 1, t + len - (n + 1), i, dim);
    }
    return n;
}

// Calculate the nearest node for a specific value
void nearest(KDnode *root, KDnode *nd, int i, int dim, KDnode **best, double *best_dist, int *visited)
{
    double d, dx, dx2;

    if (!root)
        return;
    d = dist(root, nd);

    dx = root->midpoint[i] - nd->midpoint[i];
    dx2 = dx * dx;

    (*visited)++;

    if (!*best || d < *best_dist)
    {
        *best_dist = d;
        *best = root;
    }

    // If chance of exact match is high
    if (!*best_dist)
        return;

    if (++i >= dim)
        i = 0;

    nearest(dx > 0 ? root->left : root->right, nd, i, dim, best, best_dist, visited);
    if (dx2 >= *best_dist)
        return;
    nearest(dx > 0 ? root->right : root->left, nd, i, dim, best, best_dist, visited);
}

// Count the number of nodes of binary tree
int countNodes(KDnode *root)
{
    if (root == NULL)
        return 0;
    else
        return (1 + countNodes(root->left) + countNodes(root->right));
}

// Helper struct and function to print branches of the binary tree
struct Trunk
{
    Trunk *prev;
    std::string str;
    Trunk(Trunk *prev, std::string str)
    {
        this->prev = prev;
        this->str = str;
    }
};

void showTrunks(Trunk *p)
{
    if (p == NULL)
        return;

    showTrunks(p->prev);

    std::cout << p->str;
}

// Recursive function to print binary tree. It uses inorder traversal
// call as printTree(root, nullptr, false);
void printTree(KDnode *root, Trunk *prev, bool isLeft)
{
    if (root == NULL)
        return;

    std::string prev_str = "    ";
    Trunk *trunk = new Trunk(prev, prev_str);
    printTree(root->left, trunk, true);

    if (!prev)
        trunk->str = "---";
    else if (isLeft)
    {
        trunk->str = ".---";
        prev_str = "   |";
    }
    else
    {
        trunk->str = "`---";
        prev->str = prev_str;
    }

    showTrunks(trunk);
    std::cout << "(" << root->midpoint.x() << "," << root->midpoint.y() << "," << root->midpoint.z() << ") " << std::endl;

    if (prev)
        prev->str = prev_str;

    trunk->str = "   |";
    printTree(root->right, trunk, false);
}

// A utility function to find min and max distances with respect to root.
void findMinMax(KDnode *node, int *min, int *max, int hd)
{
    // Base case
    if (node == NULL)
        return;

    // Update min and max
    if (hd < *min)
        *min = hd;
    else if (hd > *max)
        *max = hd;

    // Recur for left and right subtrees
    findMinMax(node->left, min, max, hd - 1);
    findMinMax(node->right, min, max, hd + 1);
}

// A utility function to print all nodes on a given line_no.
// hd is horizontal distance of current node with respect to root.
void printVerticalLine(KDnode *node, int line_no, int hd)
{
    // Base case
    if (node == NULL)
        return;

    // If this node is on the given line number
    if (hd == line_no)
        std::cout << "(" << node->midpoint.x() << "," << node->midpoint.y() << "," << node->midpoint.z() << ") ";

    // Recur for left and right subtrees
    printVerticalLine(node->left, line_no, hd - 1);
    printVerticalLine(node->right, line_no, hd + 1);
}

// The main function that prints a given binary tree in vertical order
void verticalOrder(KDnode *root)
{
    // Find min and max distances with respect to root
    int min = 0, max = 0;
    findMinMax(root, &min, &max, 0);

    // Iterate through all possible vertical lines starting
    // from the leftmost line and print nodes line by line
    for (int line_no = min; line_no <= max; line_no++)
    {
        printVerticalLine(root, line_no, 0);
        std::cout << std::endl;
    }
}

#define N 1000000
#define rand1() (rand() / (double)RAND_MAX)
#define rand_pt(v)                                       \
{                                                    \
    v.midpoint = osg::Vec3f(rand1(), rand1(), rand1()); \
}

BOOST_AUTO_TEST_CASE(KDTree_2d)
{
    KDnode wp[] = {
        KDnode(osg::Vec3f(2, 3, 0)),
        KDnode(osg::Vec3f(5, 4, 0)),
        KDnode(osg::Vec3f(9, 6, 0)),
        KDnode(osg::Vec3f(1, 9, 0)),
        KDnode(osg::Vec3f(3, 3, 0)),
        KDnode(osg::Vec3f(8, 1, 0)),
        KDnode(osg::Vec3f(7, 2, 0))};

    KDnode testNode(osg::Vec3f(9, 2, 0));
    KDnode *root, *found;
    double best_dist;

    root = makeTree(wp, sizeof(wp) / sizeof(KDnode), 0, 2);

    int visited = 0;
    found = 0;
    nearest(root, &testNode, 0, 2, &found, &best_dist, &visited);

    printf(">> WP tree\nsearching for (%g, %g)\n"
           "found (%g, %g) dist %g\nseen %d nodes\n\n",
           testNode.midpoint.x(), testNode.midpoint.y(),
           found->midpoint.x(), found->midpoint.y(), sqrt(best_dist), visited);

    std::cout << ">> Number of nodes: " << countNodes(root) << std::endl;

    printTree(root, NULL, false);

    std::cout << "\n>> Vertical order traversal is " << std::endl;
    verticalOrder(root);
}

BOOST_AUTO_TEST_CASE(KDTree_3d)
{
    KDnode testNode(osg::Vec3f(9, 2, 0));
    KDnode *root, *found, *million;
    double best_dist;

    million = (KDnode *)calloc(N, sizeof(KDnode));
    srand(time(0));
    for (int i = 0; i < N; i++)
        rand_pt(million[i]);

    root = makeTree(million, N, 0, 3);
    rand_pt(testNode);

    int visited = 0;
    found = 0;
    nearest(root, &testNode, 0, 3, &found, &best_dist, &visited);

    printf("\n>> Million tree\nsearching for (%g, %g, %g)\n"
           "found (%g, %g, %g) dist %g\nseen %d nodes\n",
           testNode.midpoint.x(), testNode.midpoint.y(), testNode.midpoint.z(),
           found->midpoint.x(), found->midpoint.y(), found->midpoint.z(),
           sqrt(best_dist), visited);

    /* search many random points in million tree to see average behavior.
    tree size vs avg nodes visited:
    10           ~  7
    100          ~ 16.5
    1000         ~ 25.5
    10000        ~ 32.8
    100000       ~ 38.3
    1000000      ~ 42.6
    10000000     ~ 46.7              */
    int sum = 0, test_runs = 100000;
    for (int i = 0; i < test_runs; i++)
    {
        found = 0;
        visited = 0;
        rand_pt(testNode);
        nearest(root, &testNode, 0, 3, &found, &best_dist, &visited);
        sum += visited;
    }
    printf("\n>> Million tree\n"
           "visited %d nodes for %d random findings (%f per lookup)\n",
           sum, test_runs, sum / (double)test_runs);

    free(million);
}

BOOST_AUTO_TEST_CASE(KDtree_benchmarking)
{
    /* benchmarking test */
    int test_runs = 800 * 600; // 800 * 600 px
    int triangles = 10000;     // number of triangles

    KDnode *million = (KDnode *)calloc(triangles, sizeof(KDnode));
    srand(time(0));
    for (int i = 0; i < triangles; i++)
        rand_pt(million[i]);

    KDnode *root = makeTree(million, triangles, 0, 3);

    clock_t begin = clock();
    KDnode *found;
    KDnode testNode(osg::Vec3f(9, 2, 0));
    double best_dist;
    int visited = 0;
    for (int i = 0; i < test_runs; i++)
    {
        found = 0;
        rand_pt(testNode);
        nearest(root, &testNode, 0, 3, &found, &best_dist, &visited);
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "\n>> Elapsed time: " << elapsed_secs << std::endl;
}

BOOST_AUTO_TEST_SUITE_END();
