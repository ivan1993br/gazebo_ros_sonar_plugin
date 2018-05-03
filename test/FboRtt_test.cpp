// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osgDB/ReadFile>

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

BOOST_AUTO_TEST_SUITE(FboRtt)

osg::Texture2D* createRttTexture(int width, int height) {
    // Create the texture; we'll use this as our color buffer.
    // Note it has no image data; not required.
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( width, winH );
    tex->setInternalFormat( GL_RGBA16F_ARB );
    tex->setSourceFormat(GL_RGBA);
    tex->setSourceType(GL_FLOAT);
    tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    return tex.release();
}

BOOST_AUTO_TEST_CASE(FBO_RTT_testCase) {
    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> scene = new osg::Group();
    // makeDemoScene(scene);
    osg::Node* model = osgDB::readNodeFile( "/home/romulo/Tools/OpenSceneGraph-Data/cow.osg" );
    scene->addChild(model);

    osgViewer::Viewer viewer;
    viewer.setSceneData(scene);
    viewer.run();

    osg::Vec3 eye,center,up;
    viewer.getCamera()->getViewMatrixAsLookAt(eye,center,up);
    std::cout << "--- camera params ---" << std::endl;
    std::cout << "eye    : " << eye.x() << "," << eye.y() << "," << eye.z() << std::endl;
    std::cout << "center : " << center.x() << "," << center.y() << "," << center.z() << std::endl;
    std::cout << "up     : " << up.x() << "," << up.y() << "," << up.z() << std::endl;
}

BOOST_AUTO_TEST_SUITE_END();
