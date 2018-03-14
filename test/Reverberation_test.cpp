// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TriangleFunctor>
#include <osgDB/FileUtils>

// C++ includes
#include <iostream>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Reverberation_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Reverberation)

cv::Mat setChannelValue(cv::Mat img, int channel, uchar value) {
    if (channel < 0 || channel > 2)
        std::runtime_error("Channel number must be between 1 and 3");

    std::vector<cv::Mat> channels;
    cv::split(img, channels);
    channels[channel].setTo(value);
    cv::Mat out;
    cv::merge(channels, out);

   return out;
}

struct CollectTriangles {
    CollectTriangles() {
        verts = new osg::Vec3Array();
    }

    inline void operator () (const osg::Vec3& v1,const osg::Vec3& v2,const osg::Vec3& v3, bool treatVertexDataAsTemporary) {
        verts->push_back(v1);
        verts->push_back(v2);
        verts->push_back(v3);
    }

    osg::ref_ptr< osg::Vec3Array > verts;
};


class CollectTrianglesVisitor : public osg::NodeVisitor {

    public:
        CollectTrianglesVisitor() {
            vertices = new osg::Vec3Array();
            setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
        }

        void apply( osg::Geode& geode ) {
            for ( unsigned int i = 0; i<geode.getNumDrawables(); ++i ) {
                osg::TriangleFunctor<CollectTriangles> triangleCollector;
                geode.getDrawable(i)->accept(triangleCollector);
                vertices->insert(vertices->end(), triangleCollector.verts->begin(), triangleCollector.verts->end());
            }
        }
        osg::ref_ptr< osg::Vec3Array > vertices;
};

BOOST_AUTO_TEST_CASE(reverberation_testCase) {
     // define the different camera point of views
    std::vector<osg::Vec3d> eyes, centers, ups;
    viewPointsFromDemoScene(&eyes, &centers, &ups);

    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> scene = new osg::Group();
    makeDemoScene(scene);

    // get all triangles' vertices from all scene models
    CollectTrianglesVisitor visitor;
    scene->accept(visitor);

    // until OpenGL 4.3, arrays in GLSL must be fixed, compile-time size.
    // In this case, the triangles' vertices are stored as texture and passed to shader.
    osg::ref_ptr<osg::Image> image = new osg::Image();
}

BOOST_AUTO_TEST_SUITE_END();
