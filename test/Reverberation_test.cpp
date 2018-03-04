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

#define BOOST_TEST_MODULE "Reflection_test"
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
                // std::cout << "Node: " << i << " / Vertices: " << triangleCollector.verts->size() << std::endl;
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
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(visitor.vertices->size(), 1, 1, GL_RGBA8, GL_RGBA, GL_FLOAT, (unsigned char*) &visitor.vertices[0], osg::Image::NO_DELETE);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage(image);

    // Pass the texture to GLSL as uniform
    osg::StateSet* ss = scene->getOrCreateStateSet();
    ss->addUniform( new osg::Uniform("vertexMap", texture) );

    // sonar parameters
    float maxRange = 50;      // 50 meters
    float fovX = M_PI / 6;    // 30 degrees
    float fovY = M_PI / 6;    // 30 degrees

    // display the same scene with and without underwater acoustic attenuation
    for (uint i = 0; i < eyes.size(); ++i) {
        cv::Mat rawShader = computeNormalDepthMap(scene, maxRange, fovX, fovY, 0, eyes[i], centers[i], ups[i]);
        cv::Mat rawSonar  = drawSonarImage(rawShader, maxRange, fovX * 0.5);

        // output
        cv::imshow("raw shader", rawShader);
        cv::imshow("raw sonar ", rawSonar);
        cv::waitKey();
    }
}

BOOST_AUTO_TEST_SUITE_END();
