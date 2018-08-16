// OSG includes
#include <osgViewer/Viewer>
#include <osg/TriangleFunctor>
#include <osg/io_utils>

// C++ includes
#include <iostream>
#include <vector>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Reverberation_test"
#include <boost/test/unit_test.hpp>

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
        data[0] = v1;                       // vertex 1
        data[1] = v2;                       // vertex 2
        data[2] = v3;                       // vertex 3
        data[3] = (v1 + v2 + v3) / 3;       // centroid
        data[4] = (v2 - v1) ^ (v3 - v1);    // surface normal
    };

    // get the triangle data as vector of float
    std::vector<float> getAllDataAsVector()
    {
        float* array = &data[0].x();
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

            inline void operator()( const osg::Vec3f &v1,
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

    public:
        osg::TriangleFunctor<WorldTriangle> tf;

        MyTrianglesVisitor()
        {
            setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        };

        void apply(osg::Geode &geode)
        {
            tf.local2world = osg::computeLocalToWorld(this->getNodePath());
            for (size_t i = 0; i < geode.getNumDrawables(); ++i)
                geode.getDrawable(i)->accept(tf);
        }

        std::vector<MyTriangle> getTriangles() { return tf.triangles; };
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
void printMyTriangle1(std::vector<MyTriangle>& triangles)
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

        for (size_t j = 0; j < data.size() / 3; j++) {
            std::cout << data[j * 3 + 0] << ", "
                      << data[j * 3 + 1] << ", "
                      << data[j * 3 + 2] << std::endl;
        }
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

    // sort triangles in ascending order
    std::sort(triangles.begin(), triangles.end());

    // convert triangles to texture
    // osg::ref_ptr<osg::Texture2D> texture;
    // convertTrianglesToTextures(triangles, texture);

    // print triangles data
    // printMyTriangle1(triangles);
    // printMyTriangle2(triangles);
}

BOOST_AUTO_TEST_CASE(reverberation2_testCase)
{
    std::vector<MyTriangle> triangles;
    triangles.push_back(MyTriangle(osg::Vec3f(1.2, 5.6, 0.7), osg::Vec3f(9.9, 0.1, 4.5), osg::Vec3f(1.9, 2.2, 3.4)));
    triangles.push_back(MyTriangle(osg::Vec3f(7.2, 3.6, 7.0), osg::Vec3f(2.2, 4.4, 4.3), osg::Vec3f(1.2, 2.5, 6.7)));
    triangles.push_back(MyTriangle(osg::Vec3f(8.8, 1.1, 3.3), osg::Vec3f(2.6, 0.8, 5.1), osg::Vec3f(4.0, 2.5, 5.5)));

    printMyTriangle1(triangles);
    // printMyTriangle2(triangles);
}

BOOST_AUTO_TEST_SUITE_END();
