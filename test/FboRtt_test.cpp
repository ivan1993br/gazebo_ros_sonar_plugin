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

#define BOOST_TEST_MODULE "Reverberation_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(FboRtt)

const int winW( 800 ), winH( 600 );

struct SnapImage : public osg::Camera::DrawCallback {
    SnapImage(osg::GraphicsContext* gc)
    {
        _image = new osg::Image;
        if (gc->getTraits()) {
            GLenum pixelFormat;
            if (gc->getTraits()->alpha)
                pixelFormat = GL_RGBA;
            else
                pixelFormat = GL_RGB;

            int width = gc->getTraits()->width;
            int height = gc->getTraits()->height;
            _image->allocateImage(width, height, 1, pixelFormat, GL_UNSIGNED_BYTE);
        }
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        osg::Camera* camera = renderInfo.getCurrentCamera();
        osg::GraphicsContext* gc = camera->getGraphicsContext();
        if (gc->getTraits() && _image.valid())
        {
           _image->readPixels(0,0,int(gc->getTraits()->width),int(gc->getTraits()->height),
                              GL_RGBA,
                              GL_UNSIGNED_BYTE);
            osgDB::writeImageFile(*_image,  "./Test.png");
        }

    }

    mutable osg::ref_ptr<osg::Image>    _image;
};

BOOST_AUTO_TEST_CASE(fboRtt_testCase) {
    osg::ref_ptr< osg::Group > root( new osg::Group );
    makeDemoScene(root);

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 10, 30, winW, winH );
    viewer.setSceneData( root.get() );
    viewer.realize();

    // Create the texture; we'll use this as our color buffer.
    // Note it has no image data; not required.
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( winW, winH );
    tex->setInternalFormat( GL_RGBA16F_ARB );
    tex->setSourceFormat(GL_RGBA);
    tex->setSourceType(GL_FLOAT);
    tex->setResizeNonPowerOfTwoHint( false );
    tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

    // Attach the texture to the camera. Tell it to use multisampling.
    // Internally, OSG allocates a multisampled renderbuffer, renders to it,
    // and at the end of the frame performs a BlitFramebuffer into our texture.
    osg::Camera* rootCamera( viewer.getCamera() );
    rootCamera->setClearColor(osg::Vec4());
    rootCamera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    rootCamera->setViewport( 0, 0, winW, winH );
    rootCamera->setRenderOrder( osg::Camera::PRE_RENDER );
    rootCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    rootCamera->attach( osg::Camera::COLOR_BUFFER, tex);//, 0, 0, false, 8, 8 );

    // Set RTT texture to quad
    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osg::createTexturedQuadGeometry(
        osg::Vec3(-1,-1,0), osg::Vec3(2.0,0.0,0.0), osg::Vec3(0.0,2.0,0.0)) );

    geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, tex );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getOrCreateStateSet()->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    // Configure postRenderCamera to draw fullscreen textured quad
    osg::Camera* postRenderCamera( new osg::Camera );
    postRenderCamera->setClearMask( 0 );
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->setViewMatrix( osg::Matrixd::identity() );
    postRenderCamera->setProjectionMatrix( osg::Matrixd::identity() );
    postRenderCamera->addChild( geode );

    // root->addChild( postRender( viewer ) );
    root->addChild(postRenderCamera);

    SnapImage* finalDrawCallback = new SnapImage(viewer.getCamera()->getGraphicsContext());
    rootCamera->setFinalDrawCallback(finalDrawCallback);

    viewer.run();
}

BOOST_AUTO_TEST_SUITE_END();
