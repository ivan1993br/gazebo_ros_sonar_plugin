// C++ includes
#include "Tools.hpp"
#include <cmath>

namespace normal_depth_map {

double underwaterSignalAttenuation( const double frequency,
                                    const double temperature,
                                    const double depth,
                                    const double salinity,
                                    const double acidity) {

    double frequency2 = frequency * frequency;

    // borid acid and magnesium sulphate relaxation frequencies (in kHz)
    double f1 = 0.78 * pow(salinity / 35, 0.5) * exp(temperature / 26);
    double f2 = 42 * exp(temperature / 17);

    // borid acid contribution
    double borid = 0.106 * ((f1 * frequency2) / (frequency2 + f1 * f1)) * exp((acidity - 8) / 0.56);

    // magnesium sulphate contribuion
    double magnesium = 0.52 * (1 + temperature / 43) * (salinity / 35)
                        * ((f2 * frequency2) / (frequency2 + f2 * f2)) * exp(-depth / 6000);

    // freshwater contribution
    double freshwater = 0.00049 * frequency2 * exp(-(temperature / 27 + depth / 17000));

    // absorptium attenuation coefficient in dB/km
    double attenuation = borid + magnesium + freshwater;

    // convert dB/km to dB/m
    attenuation = attenuation / 1000.0;

    // convert dB/m to Pa/m
    attenuation = pow(10, -attenuation / 20);
    attenuation = -log(attenuation);

    return attenuation;
}

TrianglesVisitor::TrianglesVisitor() {
    setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
};

void TrianglesVisitor::apply( osg::Geode& geode ) {

    for ( unsigned int i = 0; i< geode.getNumDrawables(); ++i ) {
        triangles_data.local_2_world = osg::computeLocalToWorld(getNodePath());
        geode.getDrawable(i)->accept(triangles_data);
    }
}


void convertTrianglesToTextures(
                      std::vector<TriangleStruct> triangles,
                      osg::ref_ptr<osg::Texture2D>& texture){

    osg::ref_ptr<osg::Image> image = new osg::Image();
    image->allocateImage( triangles.size(),
                          triangles[0].getAllDataAsVector().size(),
                          1,
                          GL_RED,
                          GL_FLOAT);
    image->setInternalTextureFormat(GL_R32F);

    for (unsigned int j = 0; j < triangles.size(); j++){
        std::vector<float> data = triangles[j].getAllDataAsVector();
        for (unsigned int i = 0; i < data.size(); i++)
            setOSGImagePixel(image, i, j, 0, data[i]);
    }

    texture = new osg::Texture2D;
    texture->setTextureSize(image->s(), image->t());
    texture->setResizeNonPowerOfTwoHint(false);
    texture->setUnRefImageDataAfterApply(true);
    texture->setImage(image);
}

// Find the median value of binary tree
TriangleStruct *findMedian(TriangleStruct *start, TriangleStruct *end, int idx)
{
    if (end <= start)
        return NULL;
    if (end == start + 1)
        return start;

    TriangleStruct *p, *store, *md = start + (end - start) / 2;
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

        // Median has duplicate values
        if (store->data[3][idx] == md->data[3][idx])
            return md;

        if (store > md)
            end = store;
        else
            start = store;
    }
}

// Build the k-d tree
TriangleStruct *makeTree(TriangleStruct *t, int len, int idx, int dim)
{
    TriangleStruct *n;

    if (!len)
        return 0;

    if ((n = findMedian(t, t + len, idx)))
    {
        idx = (idx + 1) % dim;
        n->left = makeTree(t, n - t, idx, dim);
        n->right = makeTree(n + 1, t + len - (n + 1), idx, dim);
    }
    return n;
}

// Compute the "maxDepth" of a tree -- the number of nodes along
// the longest path from the root node down to the farthest leaf node.
int height(TriangleStruct *node)
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

// Get nodes at a given level
void getGivenLevel(TriangleStruct *node, std::vector<TriangleStruct> &vec, int level)
{
    if (node == NULL)
        return;

    if (level == 1)
        vec.push_back(*node);

    else if (level > 1) {
        getGivenLevel(node->left, vec, level - 1);
        getGivenLevel(node->right, vec, level - 1);
    }
}

// Get nodes on level order
void levelOrder(TriangleStruct *root, std::vector<TriangleStruct>& vec) {
    int h = height(root);
    for (int i = 1; i <= h; i++)
        getGivenLevel(root, vec, i);
}

}
