#include "ImageViewerCaptureTool.hpp"
#include <iostream>
#include <unistd.h>

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

osg::Camera* ImageViewerCaptureTool::setupMRTCamera( osg::ref_ptr<osg::Camera> camera, std::vector<osg::Texture2D*>& attachedTextures, osg::ref_ptr<osg::GraphicsContext> gfxc ) {
    uint w = gfxc->getTraits()->width;
    uint h = gfxc->getTraits()->height;

    // setup camera
    camera->setClearColor( osg::Vec4() );
    camera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    camera->setGraphicsContext( gfxc );
    camera->setDrawBuffer( GL_FRONT );
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setRenderOrder( osg::Camera::PRE_RENDER, 0 );
    camera->setViewport( 0, 0, w, h );
    camera->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    camera->setViewMatrix( osg::Matrix::identity() );

    // setup rendered texture
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( w, h );
    tex->setInternalFormat( GL_RGB32F );
    tex->setSourceFormat( GL_RGBA );
    tex->setSourceType( GL_FLOAT );
    tex->setResizeNonPowerOfTwoHint( false );
    tex->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    attachedTextures.push_back( tex );
    camera->attach( osg::Camera::COLOR_BUFFER, tex );

    return camera.release();
}

void ImageViewerCaptureTool::initializeProperties(uint width, uint height, double fovY) {
    _viewer = new osgViewer::Viewer;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = width;
    traits->height = height;
    traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    // setup MRT camera
    std::vector<osg::Texture2D*> attachedTextures;
    osg::Camera* mrtCamera ( _viewer->getCamera() );
    setupMRTCamera(mrtCamera, attachedTextures, gfxc);
    mrtCamera->setProjectionMatrixAsPerspective( osg::RadiansToDegrees( fovY ), ( width * 1.0 / height ), 0.1, 1000 );

    // setup the callback to collect buffer frames
    _capture = new WindowCaptureScreen( gfxc, attachedTextures[0]);
    mrtCamera->setFinalDrawCallback( _capture );
}

osg::ref_ptr<osg::Image> ImageViewerCaptureTool::grabImage(osg::ref_ptr<osg::Node> node) {
    // set the current root node
    _viewer->setSceneData(node);
    _viewer->realize();

    // grab the current frame
    _viewer->frame();
    return _capture->captureImage();
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

WindowCaptureScreen::WindowCaptureScreen(osg::ref_ptr<osg::GraphicsContext> gfxc, osg::Texture2D* tex) {
    _mutex = new OpenThreads::Mutex();
    _condition = new OpenThreads::Condition();
    _image = new osg::Image();

    // checks the GraficContext from the camera viewer
    if (gfxc->getTraits()) {
        _tex = tex;
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

void WindowCaptureScreen::operator ()(osg::RenderInfo& renderInfo) const {
    osg::ref_ptr<osg::GraphicsContext> gfxc = renderInfo.getState()->getGraphicsContext();

    if (gfxc->getTraits()) {
        _mutex->lock();

        // read the color buffer
        renderInfo.getState()->applyTextureAttribute(0, _tex);
        _image->readImageFromCurrentTexture(renderInfo.getContextID(), true, GL_FLOAT);

        //grants the access to image
        _condition->signal();
        _mutex->unlock();
    }
}

}
