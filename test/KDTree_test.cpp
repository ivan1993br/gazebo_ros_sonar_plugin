// C/C++ includes
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <ctime>
#include <iostream>
#include <string>
#include <queue>
#include <stack>

// OSG includes
#include <osg/Geode>
#include <osg/Geometry>

#define BOOST_TEST_MODULE "KDTree_test"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_KDTree)

// node of k-d tree
struct KDnode
{
    std::vector<osg::Vec3> data;
    KDnode *left, *right;

    KDnode()
        : data(5, osg::Vec3(0,0,0))
        , left(NULL)
        , right(NULL) {};

    KDnode(osg::Vec3 k)
        : data(5, osg::Vec3(0,0,0))
        , left(NULL)
        , right(NULL) {
            data[3] = k;
        };

    KDnode(osg::Vec3 v1, osg::Vec3 v2, osg::Vec3 v3)
        : data(5, osg::Vec3(0,0,0))
        , left(NULL)
        , right(NULL) {
        setTriangle(v1, v2, v3);
    };

    void setTriangle(osg::Vec3 v1, osg::Vec3 v2, osg::Vec3 v3) {
        data[0] = v1;                                  // vertex 1
        data[1] = v2;                                  // vertex 2
        data[2] = v3;                                  // vertex 3
        data[3] = (v1 + v2 + v3) / 3;                  // centroid
        data[4] = (v2 - v1).operator ^(v3 - v1);
        data[4].normalize();                           // normal of triangle's plane
    };
};

// Calculate the distance between two points
inline double dist(KDnode *a, KDnode *b)
{
    osg::Vec3 c = a->data[3] - b->data[3];
    return (c.x() * c.x()
            + c.y() * c.y()
            + c.z() * c.z());
}

// Switch the values of node A and B
inline void swap(KDnode *a, KDnode *b)
{
    std::vector<osg::Vec3> tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

// Find the median value of binary tree
KDnode *findMedian(KDnode *start, KDnode *end, int idx)
{
    if (end <= start)
        return NULL;
    if (end == (start + 1))
        return start;

    KDnode *p, *store, *md = start + (end - start) / 2;
    double pivot;
    while (1)
    {
        pivot = md->data[3][idx];

        swap(md, end - 1);
        for (store = p = start; p < end; p++)
        {
            if (p->data[3][idx] < pivot)
            {
                if (p != store)
                    swap(p, store);
                store++;
            }
        }
        swap(store, end - 1);

        // Median has duplicate values */
        if (store->data[3][idx] == md->data[3][idx])
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

// Calculate the nearest node for a specific value (using node)
void nearest1(KDnode *root, KDnode *nd, int i, int dim, KDnode **best, double *best_dist, int *visited)
{
    if (!root)
        return;

    double d = dist(root, nd);
    double dx = root->data[3][i] - nd->data[3][i];
    double dx2 = dx * dx;

    std::cout << "\nVisited: (" << root->data[3].x() << "," << root->data[3].y() << "," << root->data[3].z() << ") " << std::endl;

    (*visited)++;

    if (!*best || d < *best_dist)
    {
        *best_dist = d;
        *best = root;
    }

    // If chance of exact match is high
    if (!*best_dist)
        return;

    i = ++i % dim;

    std::cout << "d  = " << d << std::endl;
    std::cout << "dx = " << dx << std::endl;
    std::cout << "dx2 = " << dx2 << std::endl;
    std::cout << "best dist = " << *best_dist << std::endl;

    nearest1(dx > 0 ? root->left : root->right, nd, i, dim, best, best_dist, visited);

    if (dx2 < *best_dist) {
        std::cout << "dx2 < best_dist" << std::endl;
        nearest1(dx > 0 ? root->right : root->left, nd, i, dim, best, best_dist, visited);
    }
}

// Calculate the nearest node for a specific value (using vector)
void nearest2(const std::vector<float>& vec, KDnode *nd, int idx, int i, int dim, KDnode **best, double *best_dist, int *visited)
{
    if (idx > (vec.size() - 1))
        return;

    KDnode *root = new KDnode(osg::Vec3(vec[idx + 9], vec[idx + 10], vec[idx + 11]));

    double d = dist(root, nd);
    double dx = root->data[3][i] - nd->data[3][i];

    (*visited)++;

    if (!*best || d < *best_dist)
    {
        *best_dist = d;
        *best = root;
    }

    // If chance of exact match is high
    if (!*best_dist)
        return;

    i = ++i % dim;

    int idx_left = 2 * idx + 15;
    int idx_right = 2 * idx + 30;

    nearest2(vec, nd, dx > 0 ? idx_left : idx_right, i, dim, best, best_dist, visited);

    double dx2 = dx * dx;
    if (dx2 < *best_dist)
        nearest2(vec, nd, dx > 0 ? idx_right : idx_left, i, dim, best, best_dist, visited);
}

// Calculate the nearest node for a specific value (using vector by iterative way)
void nearest3(const std::vector<float> &vec, KDnode *nd, int dim, KDnode **best, double *best_dist, int *visited)
{
    KDnode *root = new KDnode();

    std::queue<int> stack;
    stack.push(0);
    int i = 0;

    while (!stack.empty()) {
        int idx = stack.front();
        stack.pop();

        if (idx > (vec.size() - 1))
            continue;

        root->data[3].set(vec[idx + 9], vec[idx + 10], vec[idx + 11]);

        double d = dist(root, nd);
        double dx = root->data[3][i] - nd->data[3][i];
        double dx2 = dx * dx;

        (*visited)++;

        if (!*best || d < *best_dist)
        {
            *best_dist = d;
            *best = root;
        }

        // If chance of exact match is high
        if (!*best_dist)
            return;

        i = ++i % dim;

        int idx_left = 2 * idx + 15;
        int idx_right = 2 * idx + 30;

        stack.push(dx > 0 ? idx_left : idx_right);
    }
}

// A structure to represent a queue
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int *array;
};

// function to create a queue of given capacity. It initializes size of
// queue as 0
Queue *createQueue(unsigned capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1; // This is important, see the enqueue
    queue->array = (int *)malloc(queue->capacity * sizeof(int));
    return queue;
}

// Queue is full when size becomes equal to the capacity
int isFull(Queue *queue)
{
    return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty(Queue *queue)
{
    return (queue->size == 0);
}

// Function to add an item to the queue.  It changes rear and size
void enqueue(Queue *queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

// Function to remove an item from queue.  It changes front and size
int dequeue(Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
int front(Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}

// Function to get rear of queue
int rear(Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}

void nearest4(const std::vector<float> &vec, KDnode *nd, int dim, KDnode **best, double *best_dist, int *visited)
{
    KDnode *root = new KDnode();

    Queue *queue = createQueue(1024);
    enqueue(queue, 0);
    int i = 0;

    while (!isEmpty(queue))
    {
        int idx = dequeue(queue);

        if (idx > (vec.size() - 1))
            continue;

        root->data[3].set(vec[idx + 9], vec[idx + 10], vec[idx + 11]);

        double d = dist(root, nd);
        double dx = root->data[3][i] - nd->data[3][i];

        (*visited)++;

        if (!*best || d < *best_dist)
        {
            *best_dist = d;
            *best = root;
        }

        // If chance of exact match is high
        if (!*best_dist)
            return;

        i = ++i % dim;

        int idx_left = 2 * idx + 15;
        int idx_right = 2 * idx + 30;

        enqueue(queue, (dx > 0 ? idx_left : idx_right));
    }
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
    std::cout << "(" << root->data[3].x() << "," << root->data[3].y() << "," << root->data[3].z() << ") " << std::endl;

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
    if (hd == line_no) {
        std::cout << "(" << node->data[3].x() << "," << node->data[3].y() << "," << node->data[3].z() << ") ";
    }

    // Recur for left and right subtrees
    printVerticalLine(node->left, line_no, hd - 1);
    printVerticalLine(node->right, line_no, hd + 1);
}

// The main function that prints a given binary tree in vertical order
void printVerticalOrder(KDnode *root)
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

void getVerticalNodes(KDnode *node, std::vector<float>& vec, int line_no, int hd)
{
    // Base case
    if (node == NULL)
        return;

    // If this node is on the given line number
    if (hd == line_no) {
        for (int i = 0; i < node->data.size(); i++) {
            for (int j = 0; j < 3; j++) {
                vec.push_back(node->data[i][j]);
            }
        }
    }

    // Recur for left and right subtrees
    getVerticalNodes(node->left, vec, line_no, hd - 1);
    getVerticalNodes(node->right, vec, line_no, hd + 1);
}

// Compute the "maxDepth" of a tree -- the number of nodes along
// the longest path from the root node down to the farthest leaf node.
int height(KDnode *node)
{
    if (node == NULL)
        return 0;
    else
    {
        /* compute the depth of each subtree */
        int lDepth = height(node->left);
        int rDepth = height(node->right);

        /* use the larger one */
        if (lDepth > rDepth)
            return (lDepth + 1);
        else
            return (rDepth + 1);
    }
}

// Print nodes at a given level
void printGivenLevel(KDnode *root, int level)
{
    if (root == NULL)
        return;
    if (level == 1)
        std::cout << "(" << root->data[3].x() << "," << root->data[3].y() << "," << root->data[3].z() << ") ";
    else if (level > 1)
    {
        printGivenLevel(root->left, level - 1);
        printGivenLevel(root->right, level - 1);
    }
}

// Function to print level order traversal a tree
void printLevelOrder(KDnode *node)
{
    int h = height(node);
    for (int i = 1; i <= h; i++)
        printGivenLevel(node, i);
}

// Print nodes at a given level
void getGivenLevel(KDnode *root, std::vector<float>& vec, int level)
{
    if (root == NULL)
        return;

    if (level == 1) {
        for (int i = 0; i < root->data.size(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                vec.push_back(root->data[i][j]);
            }
        }
    } else if (level > 1)
    {
        getGivenLevel(root->left, vec, level - 1);
        getGivenLevel(root->right, vec, level - 1);
    }
}

void kdtree2vector(KDnode *root, std::vector<float> &vec)
{
    int h = height(root);

    for (int i = 1; i <= h; i++)
        getGivenLevel(root, vec, i);
}

#define N 1000000
#define rand1() (rand() / (double)RAND_MAX)
#define rand_pt(v)                                      \
{                                                       \
    v = KDnode(osg::Vec3(rand1(), rand1(), rand1()));   \
}

BOOST_AUTO_TEST_CASE(KDTree_2d)
{
    std::vector<KDnode> wp;
    wp.push_back(KDnode(osg::Vec3(2, 3, 0)));
    wp.push_back(KDnode(osg::Vec3(5, 4, 0)));
    wp.push_back(KDnode(osg::Vec3(9, 6, 0)));
    wp.push_back(KDnode(osg::Vec3(1, 9, 0)));
    wp.push_back(KDnode(osg::Vec3(3, 3, 0)));
    wp.push_back(KDnode(osg::Vec3(8, 1, 0)));
    wp.push_back(KDnode(osg::Vec3(7, 2, 0)));

    KDnode testNode(osg::Vec3(9, 2, 0));
    KDnode *root, *found;
    double best_dist;

    root = makeTree(&wp[0], wp.size(), 0, 2);

    std::cout << ">> Number of nodes: " << countNodes(root) << std::endl;

    std::cout << "\n>> Height of tree: " << height(root) << std::endl;

    printTree(root, NULL, false);

    std::cout << "\n>> Vertical order traversal is " << std::endl;
    printVerticalOrder(root);

    std::cout << "\n>> Level order traversal is " << std::endl;
    printLevelOrder(root);

    std::vector<float> vec;
    kdtree2vector(root, vec);
    std::cout << "\n\n>> Size of vec: " << vec.size() << std::endl;

    int visited = 0;
    found = 0;
    // nearest1(root, &testNode, 0, 2, &found, &best_dist, &visited);
    // nearest2(vec, &testNode, 0, 0, 2, &found, &best_dist, &visited);
    // nearest3(vec, &testNode, 2, &found, &best_dist, &visited);
    nearest4(vec, &testNode, 2, &found, &best_dist, &visited);

    printf("\n>> WP tree\nsearching for (%g, %g)\n"
           "found (%g, %g) dist %g\nseen %d nodes\n\n",
           testNode.data[3].x(), testNode.data[3].y(),
           found->data[3].x(), found->data[3].y(), sqrt(best_dist), visited);

    std::cout << "================================================================\n\n" << std::endl;
}

BOOST_AUTO_TEST_CASE(KDTree_3d)
{
    KDnode testNode;
    KDnode *root, *found, *million;
    double best_dist;

    million = (KDnode *)calloc(N, sizeof(KDnode));
    srand(time(0));
    for (int i = 0; i < N; i++)
        rand_pt(million[i]);

    std::cout << "Building tree..." << std::endl;
    root = makeTree(million, N, 0, 3);
    rand_pt(testNode);

    std::cout << "Searching nearest node..." << std::endl;
    int visited = 0;
    found = 0;

    std::vector<float> vec;
    kdtree2vector(root, vec);

    // nearest1(root, &testNode, 0, 3, &found, &best_dist, &visited);
    // nearest2(vec, &testNode, 0, 0, 3, &found, &best_dist, &visited);
    // nearest3(vec, &testNode, 3, &found, &best_dist, &visited);
    nearest4(vec, &testNode, 4, &found, &best_dist, &visited);

    printf("\n>> Million tree\nsearching for (%g, %g, %g)\n"
           "found (%g, %g, %g) dist %g\nseen %d nodes\n",
           testNode.data[3].x(), testNode.data[3].y(), testNode.data[3].z(),
           found->data[3].x(), found->data[3].y(), found->data[3].z(),
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
        // nearest1(root, &testNode, 0, 3, &found, &best_dist, &visited);
        // nearest2(vec, &testNode, 0, 0, 3, &found, &best_dist, &visited);
        // nearest3(vec, &testNode, 3, &found, &best_dist, &visited);
        nearest4(vec, &testNode, 3, &found, &best_dist, &visited);
        sum += visited;
    }
    printf("\n>> Million tree\n"
           "visited %d nodes for %d random findings (%f per lookup)\n",
           sum, test_runs, sum / (double)test_runs);

    // std::cout << "\n>> Height of tree: " << height(root) << std::endl;

    free(million);
}

// BOOST_AUTO_TEST_CASE(KDtree_benchmarking)
// {
//     /* benchmarking test */
//     int test_runs = 800 * 600; // 800 * 600 px
//     int triangles = 10000;     // number of triangles

//     KDnode *million = (KDnode *)calloc(triangles, sizeof(KDnode));
//     srand(time(0));
//     for (int i = 0; i < triangles; i++)
//         rand_pt(million[i]);

//     KDnode *root = makeTree(million, triangles, 0, 3);

//     clock_t begin = clock();
//     KDnode *found;
//     KDnode testNode(osg::Vec3(9, 2, 0));
//     double best_dist;
//     int visited = 0;
//     for (int i = 0; i < test_runs; i++)
//     {
//         found = 0;
//         rand_pt(testNode);
//         nearest(root, &testNode, 0, 3, &found, &best_dist, &visited);
//     }
//     clock_t end = clock();
//     double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//     std::cout << "\n>> Elapsed time: " << elapsed_secs << std::endl;
// }

// int random(int n, int m)
// {
//     return rand() % (m - n + 1) + n;
// }

// BOOST_AUTO_TEST_CASE(KDtree_vector_search)
// {
//     std::vector<KDnode> wp;
//     for (int i = 0; i < 15; i++) {
//         osg::Vec3 point(random(-10, 10), random(-10, 10), random(-10, 10));
//         wp.push_back(point);
//     }

//     // KDnode testNode(osg::Vec3(9, 2, 0));
//     KDnode *root, *found;
//     double best_dist;

//     root = makeTree(&wp[0], wp.size(), 0, 3);

//     // int visited = 0;
//     // found = 0;
//     // nearest(root, &testNode, 0, 2, &found, &best_dist, &visited);

//     // printf(">> WP tree\nsearching for (%g, %g)\n"
//     //        "found (%g, %g) dist %g\nseen %d nodes\n\n",
//     //        testNode.data[3].x(), testNode.data[3].y(),
//     //        found->data[3].x(), found->data[3].y(), sqrt(best_dist), visited);

//     // std::cout << ">> Number of nodes: " << countNodes(root) << std::endl;

//     printTree(root, NULL, false);

//     std::cout << "\n>> Vertical order traversal is " << std::endl;
//     printVerticalOrder(root);

//     std::vector<float> vec;
//     kdtree2vector(root, vec);
//     std::cout << "\n>> Size of vec: " << vec.size() << std::endl;

// }

BOOST_AUTO_TEST_SUITE_END();
