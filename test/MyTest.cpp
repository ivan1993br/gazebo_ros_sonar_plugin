// OSG includes
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>

// C++ includes
#include <string>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "FboMrt_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(FboMrt)

osg::Camera* setupMRTCamera( osg::ref_ptr<osg::Camera> camera, std::vector<osg::Texture2D*>& attachedTextures, int w, int h ) {
    camera->setClearColor( osg::Vec4() );
    camera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setRenderOrder( osg::Camera::PRE_RENDER );
    camera->setViewport( 0, 0, w, h );

    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( w, h );
    tex->setInternalFormat( GL_RGB32F_ARB );
    tex->setSourceFormat( GL_RGBA );
    tex->setSourceType( GL_FLOAT );
    tex->setResizeNonPowerOfTwoHint( false );
    tex->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    attachedTextures.push_back( tex );
    camera->attach( osg::Camera::COLOR_BUFFER, tex );

    tex = new osg::Texture2D;
    tex->setTextureSize( w, h );
    tex->setInternalFormat( GL_DEPTH_COMPONENT );
    tex->setSourceFormat( GL_DEPTH_COMPONENT );
    tex->setSourceType( GL_UNSIGNED_BYTE );
    tex->setResizeNonPowerOfTwoHint( false );
    attachedTextures.push_back( tex );
    camera->attach( osg::Camera::DEPTH_BUFFER, tex );
    return camera.release();
}

BOOST_AUTO_TEST_CASE(fboRtt_testCase) {
    osg::ref_ptr< osg::Group > root( new osg::Group );
    makeDemoScene2(root);
    unsigned int winW = 800;
    unsigned int winH = 600;

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 0, 0, winW, winH );
    viewer.setSceneData( root.get() );
    viewer.realize();

    // setup MRT camera and textures
    std::vector<osg::Texture2D*> attachedTextures;
    osg::Camera* mrtCamera ( viewer.getCamera() );
    setupMRTCamera( mrtCamera, attachedTextures, winW, winH );

    // Set RTT texture to quad
    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osg::createTexturedQuadGeometry(osg::Vec3(-1,-1,0), osg::Vec3(2.0,0.0,0.0), osg::Vec3(0.0,2.0,0.0)) );
    geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, attachedTextures[0] );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getOrCreateStateSet()->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    // Configure postRenderCamera to draw fullscreen textured quad
    osg::Camera* postRenderCamera( new osg::Camera );
    postRenderCamera->setClearMask( 0 );
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->addChild( geode );

    root->addChild(postRenderCamera);

    viewer.run();

    osg::Vec3f eye, center, up;
    viewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
    std::cout << "--- camera params ---" << std::endl;
    std::cout << "eye    : " << eye.x() << "," << eye.y() << "," << eye.z() << std::endl;
    std::cout << "center : " << center.x() << "," << center.y() << "," << center.z() << std::endl;
    std::cout << "up     : " << up.x() << "," << up.y() << "," << up.z() << std::endl;
}

BOOST_AUTO_TEST_SUITE_END();
