#include "ImageViewerCaptureTool.hpp"
#include <iostream>
#include <unistd.h>
#include <osg/Texture2D>

namespace normal_depth_map {

ImageViewerCaptureTool::ImageViewerCaptureTool(uint width, uint height) {
    // initialize the hide viewer;
    initializeProperties(width, height);
}

ImageViewerCaptureTool::ImageViewerCaptureTool( double fovY, double fovX,
                                                uint value, bool isHeight) {
    uint width, height;

    if (isHeight) {
        height = value;
        width = height * tan(fovX * 0.5) / tan(fovY * 0.5);
    } else {
        width = value;
        height = width * tan(fovY * 0.5) / tan(fovX * 0.5);
    }

    initializeProperties(width, height, fovY);
}

void ImageViewerCaptureTool::initializeProperties(uint width, uint height, double fovY) {
    _viewer = new osgViewer::Viewer;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = width;
    traits->height = height;
    traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    // setup RTT texture
    osg::Texture2D* rttTexture = new osg::Texture2D;
    rttTexture->setTextureSize( width, height );
    rttTexture->setInternalFormat( GL_RGBA32F_ARB );
    rttTexture->setSourceFormat( GL_RGBA );
    rttTexture->setSourceType( GL_FLOAT );
    rttTexture->setResizeNonPowerOfTwoHint( false );
    rttTexture->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    rttTexture->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );

    // render camera to texture, with frame buffer
    osg::Camera* rttCamera = _viewer->getCamera();
    rttCamera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    rttCamera->setGraphicsContext( gfxc );
    rttCamera->setDrawBuffer( GL_FRONT );
    rttCamera->setReadBuffer( GL_FRONT );
    rttCamera->setRenderOrder( osg::Camera::PRE_RENDER );
    rttCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    rttCamera->setViewport( 0, 0, width, height );
    rttCamera->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    rttCamera->setProjectionMatrixAsPerspective( osg::RadiansToDegrees( fovY ), ( width * 1.0 / height ), 0.1, 1000 );
    rttCamera->attach( osg::Camera::COLOR_BUFFER, rttTexture );

    // set RTT texture to quad
    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osg::createTexturedQuadGeometry(
        osg::Vec3(-1,-1,0), osg::Vec3(2.0,0.0,0.0), osg::Vec3(0.0,2.0,0.0)) );
    geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, rttTexture );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    // display the scene in post render camera
    _postRenderCamera = new osg::Camera;
    _postRenderCamera->setClearMask( 0 );
    _postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
    _postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    _postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    _postRenderCamera->setViewMatrix( osg::Matrixd::identity() );
    _postRenderCamera->addChild( geode );

    // initialize the class to get the image in float data resolution
    _capture = new WindowCaptureScreen( gfxc );
    rttCamera->setFinalDrawCallback( _capture );
}

osg::ref_ptr<osg::Image> ImageViewerCaptureTool::grabImage(osg::ref_ptr<osg::Node> node) {
    // setup the final OSG scene
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->addChild( _postRenderCamera );
    root->addChild( node );

    // set the current root node
    _viewer->setSceneData(root.get());
    _viewer->realize();

    // grab the current frame
    _viewer->frame();
    return _capture->captureImage();
}

osg::ref_ptr<osg::Image> ImageViewerCaptureTool::getDepthBuffer() {
    return _capture->getDepthBuffer();
}


void ImageViewerCaptureTool::setCameraPosition( const osg::Vec3d& eye,
                                                const osg::Vec3d& center,
                                                const osg::Vec3d& up) {

    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
}

void ImageViewerCaptureTool::getCameraPosition( osg::Vec3d& eye,
                                                osg::Vec3d& center,
                                                osg::Vec3d& up) {

    _viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
}

void ImageViewerCaptureTool::setBackgroundColor(osg::Vec4d color) {
    _viewer->getCamera()->setClearColor(color);
}

////////////////////////////////
////WindowCaptureScreen METHODS
////////////////////////////////

WindowCaptureScreen::WindowCaptureScreen(osg::ref_ptr<osg::GraphicsContext> gc) {
    _mutex = new OpenThreads::Mutex();
    _condition = new OpenThreads::Condition();
    _image = new osg::Image();
    _depth_buffer = new osg::Image();

    // checks the GraficContext from the camera viewer
    if (gc->getTraits()) {
        GLenum pixelFormat;
        if (gc->getTraits()->alpha)
            pixelFormat = GL_RGBA;
        else
            pixelFormat = GL_RGB;

        int width = gc->getTraits()->width;
        int height = gc->getTraits()->height;

        // allocates the image memory space
        _image->allocateImage(width, height, 1, pixelFormat, GL_FLOAT);
        _depth_buffer->allocateImage(width, height, 1,  GL_DEPTH_COMPONENT, GL_FLOAT);
    }
}

WindowCaptureScreen::~WindowCaptureScreen() {
    delete (_condition);
    delete (_mutex);
}

osg::ref_ptr<osg::Image> WindowCaptureScreen::captureImage() {
    //wait to finish the capture image in call back
    _condition->wait(_mutex);
    return _image;
}

osg::ref_ptr<osg::Image> WindowCaptureScreen::getDepthBuffer() {
    return _depth_buffer;
}

void WindowCaptureScreen::operator ()(osg::RenderInfo& renderInfo) const {
    osg::ref_ptr<osg::GraphicsContext> gc = renderInfo.getState()->getGraphicsContext();
    if (gc->getTraits()) {
        _mutex->lock();
        _image->readPixels( 0, 0, _image->s(), _image->t(), _image->getPixelFormat(), GL_FLOAT);
        _depth_buffer->readPixels(0, 0, _image->s(), _image->t(), _depth_buffer->getPixelFormat(), GL_FLOAT);

        //grants the access to image
        _condition->signal();
        _mutex->unlock();
    }
}

}
