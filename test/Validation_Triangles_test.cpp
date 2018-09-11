// OSG includes
#include <osgViewer/Viewer>
#include <osg/TriangleFunctor>
#include <osg/io_utils>

// C++ includes
#include <iostream>
#include <vector>
#include <numeric>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Reverberation_test"
#include <boost/test/unit_test.hpp>

#define myrand ((float)(random()) / (float)(RAND_MAX))

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Validation)

struct MyTriangle
{
    std::vector<osg::Vec3f> data;

    MyTriangle()
        : data(5, osg::Vec3f(0, 0, 0)){};

    MyTriangle(osg::Vec3f v1, osg::Vec3f v2, osg::Vec3f v3)
        : data(5, osg::Vec3f(0, 0, 0))
    {
        setTriangle(v1, v2, v3);
    };

    void setTriangle(osg::Vec3f v1, osg::Vec3f v2, osg::Vec3f v3)
    {
        data[0] = v1;                    // vertex 1
        data[1] = v2;                    // vertex 2
        data[2] = v3;                    // vertex 3
        data[3] = (v1 + v2 + v3) / 3;    // centroid
        data[4] = (v2 - v1) ^ (v3 - v1); // surface normal
    };

    // get the triangle data as vector of float
    std::vector<float> getAllDataAsVector()
    {
        float *array = &data[0].x();
        uint arraySize = data.size() * data[0].num_components;
        std::vector<float> output(array, array + arraySize);
        return output;
    }

    bool operator<(const MyTriangle &obj_1)
    {
        return data[3] < obj_1.data[3];
    }
};

class MyTrianglesVisitor : public osg::NodeVisitor
{
  protected:
    struct WorldTriangle
    {
        std::vector<MyTriangle> triangles;
        osg::Matrixd local2world;

        inline void operator()(const osg::Vec3f &v1,
                               const osg::Vec3f &v2,
                               const osg::Vec3f &v3,
                               bool treatVertexDataAsTemporary)
        {
            // transform vertice coordinates to world coordinates
            osg::Vec3f v1_w = v1 * local2world;
            osg::Vec3f v2_w = v2 * local2world;
            osg::Vec3f v3_w = v3 * local2world;
            triangles.push_back(MyTriangle(v1_w, v2_w, v3_w));
        };
    };
    osg::TriangleFunctor<WorldTriangle> tf;
    std::vector<uint> trianglesRef;

  public:
    MyTrianglesVisitor()
    {
        setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        trianglesRef.push_back(0);
    };

    void apply(osg::Geode &geode)
    {
        tf.local2world = osg::computeLocalToWorld(this->getNodePath());
        for (size_t idx = 0; idx < geode.getNumDrawables(); ++idx) {
            geode.getDrawable(idx)->accept(tf);
            trianglesRef.push_back(tf.triangles.size());
        }
    }

    std::vector<MyTriangle> getTriangles() { return tf.triangles; };
    std::vector<uint> getTrianglesRef() { return trianglesRef; };
};

// convert triangles into texture (to be read by shader)
void triangles2texture(
    std::vector<MyTriangle> triangles,
    osg::ref_ptr<osg::Texture2D> &texture)
{
    osg::ref_ptr<osg::Image> image = new osg::Image();
    image->allocateImage(triangles.size(),
                         triangles[0].getAllDataAsVector().size(),
                         1,
                         GL_RED,
                         GL_FLOAT);
    image->setInternalTextureFormat(GL_R32F);

    for (size_t j = 0; j < triangles.size(); j++)
    {
        std::vector<float> data = triangles[j].getAllDataAsVector();
        for (size_t i = 0; i < data.size(); i++)
            setOSGImagePixel(image, i, j, 0, data[i]);
    }

    texture = new osg::Texture2D;
    texture->setTextureSize(image->s(), image->t());
    texture->setResizeNonPowerOfTwoHint(false);
    texture->setUnRefImageDataAfterApply(true);
    texture->setImage(image);
}

// print triangles data
void printMyTriangle1(std::vector<MyTriangle> &triangles)
{
    for (size_t i = 0; i < triangles.size(); i++)
    {
        std::cout << "\nTriangle " << i << ":" << std::endl;
        MyTriangle triangle = triangles[i];

        for (size_t j = 0; j < triangle.data.size(); j++)
        {
            std::cout << triangle.data[j].x() << ", "
                      << triangle.data[j].y() << ", "
                      << triangle.data[j].z() << std::endl;
        }
    }
}

// print triangles data from vector of float
void printMyTriangle2(std::vector<MyTriangle> &triangles)
{
    for (size_t i = 0; i < triangles.size(); i++)
    {
        std::cout << "\nTriangle " << i << ":" << std::endl;
        std::vector<float> data = triangles[i].getAllDataAsVector();

        for (size_t j = 0; j < data.size() / 3; j++)
        {
            std::cout << data[j * 3 + 0] << ", "
                      << data[j * 3 + 1] << ", "
                      << data[j * 3 + 2] << std::endl;
        }
    }
}

// print triangles data (on;y centroid)
void printMyTriangle3(std::vector<MyTriangle> &triangles)
{
    for (size_t i = 0; i < triangles.size(); i++)
    {
        std::cout << "\nTriangle " << i << ":" << std::endl;
        MyTriangle triangle = triangles[i];

        std::cout << triangle.data[3].x() << ", "
                  << triangle.data[3].y() << ", "
                  << triangle.data[3].z() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(reverberation_testCase)
{
    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> scene = new osg::Group();
    makeDemoScene2(scene);

    // sonar parameters
    float maxRange = 15;   // 15 meters
    float fovX = M_PI / 4; // 45 degrees
    float fovY = M_PI / 4; // 45 degrees

    // define the different camera point of views
    std::vector<osg::Vec3d> eyes, centers, ups;
    viewPointsFromDemoScene2(&eyes, &centers, &ups);

    // compute and display the final shader and sonar images
    for (unsigned i = 0; i < eyes.size(); ++i)
    {
        cv::Mat shaderImg = computeNormalDepthMap(scene, maxRange, fovX, fovY, 0, eyes[i], centers[i], ups[i]);
        cv::imshow("shaderImg", shaderImg);
        cv::waitKey();
    }

    MyTrianglesVisitor visitor;
    scene->accept(visitor);

    std::vector<MyTriangle> triangles = visitor.getTriangles();
    std::vector<uint> trianglesRef = visitor.getTrianglesRef();

    std::cout << "\nTriangles Reference" << std::endl;
    for(size_t idx = 0; idx < trianglesRef.size(); idx++)
        std::cout << trianglesRef[idx] << std::endl;

    // sort triangles of the same model
    for(size_t idx = 0; idx < trianglesRef.size() - 1; idx++)
        std::sort(triangles.begin() + trianglesRef[idx], triangles.begin() + trianglesRef[idx + 1]);

    // convert triangles to texture
    // osg::ref_ptr<osg::Texture2D> texture;
    // convertTrianglesToTextures(triangles, texture);

    // print triangles data
    // printMyTriangle1(triangles);
    // printMyTriangle2(triangles);
}

BOOST_AUTO_TEST_CASE(range_sort_testCase)
{
    std::vector<MyTriangle> triangles;

    for(size_t i = 0; i < 12; i++)
    {
        osg::Vec3f v1(myrand, myrand, myrand);
        osg::Vec3f v2(myrand, myrand, myrand);
        osg::Vec3f v3(myrand, myrand, myrand);
        triangles.push_back(MyTriangle(v1, v2, v3));
    }

    // original vector
    std::cout << "\n=== ORIGINAL VECTOR ===" << std::endl;
    printMyTriangle3(triangles);

    // sorted vector
    std::cout << "\n=== SORTED VECTOR ===" << std::endl;
    int myInts[] = {0, 5, 9, 12};
    for (size_t idx = 0; idx < 3; idx++)
        std::sort(triangles.begin() + myInts[idx], triangles.begin() + myInts[idx + 1]);
    printMyTriangle3(triangles);
}

BOOST_AUTO_TEST_SUITE_END();