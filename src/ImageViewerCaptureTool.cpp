#include "ImageViewerCaptureTool.hpp"
#include <iostream>
#include <unistd.h>

namespace normal_depth_map {

ImageViewerCaptureTool::ImageViewerCaptureTool(osg::ref_ptr<osg::Node> node, uint width, uint height) {
    // initialize the hide viewer;
    initializeProperties(node, width, height);
}

ImageViewerCaptureTool::ImageViewerCaptureTool( osg::ref_ptr<osg::Node> node, double fovY, double fovX,
                                                uint value, bool isHeight) {
    uint width, height;

    if (isHeight) {
        height = value;
        width = height * tan(fovX * 0.5) / tan(fovY * 0.5);
    } else {
        width = value;
        height = width * tan(fovY * 0.5) / tan(fovX * 0.5);
    }

    initializeProperties(node, width, height, fovY);
}

// create a RTT (render to texture) camera
osg::Camera* ImageViewerCaptureTool::createRTTCamera( osg::Camera::BufferComponent buffer, osg::Texture2D* tex, osg::Camera* cam )
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearColor( osg::Vec4() );
    camera->setClearMask( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setRenderOrder( osg::Camera::PRE_RENDER );
    camera->setViewport( 0, 0, tex->getTextureWidth(), tex->getTextureHeight() );
    camera->attach( buffer, tex );
    return camera.release();
}

// create the post render camera
osg::Camera* ImageViewerCaptureTool::createHUDCamera(osg::Camera::BufferComponent buffer, osg::Texture2D* tex)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearMask(0);
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
    camera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    camera->setRenderOrder( osg::Camera::POST_RENDER );
    camera->setViewMatrix( osg::Matrixd::identity() );
    camera->attach( buffer, tex );
    return camera.release();
}

// create float textures to be rendered in FBO
osg::Texture2D* ImageViewerCaptureTool::createFloatTexture(uint w, uint h)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize( w, h );
    tex2D->setInternalFormat( GL_RGB32F_ARB );
    tex2D->setSourceFormat( GL_RGBA );
    tex2D->setSourceType( GL_FLOAT );
    tex2D->setResizeNonPowerOfTwoHint( false );
    tex2D->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex2D->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    return tex2D.release();
}

void ImageViewerCaptureTool::initializeProperties(osg::ref_ptr<osg::Node> node, uint width, uint height, double fovY) {
    _viewer = new osgViewer::Viewer;
    _viewer->setSceneData(node);

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = width;
    traits->height = height;
    traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    // first pass
    osg::Texture2D* pass12tex = createFloatTexture(width, height);
    osg::ref_ptr<osg::Camera> pass1camera = createRTTCamera(osg::Camera::COLOR_BUFFER0, pass12tex, _viewer->getCamera());
    pass1camera->setGraphicsContext(gfxc);
    pass1camera->setDrawBuffer( GL_FRONT );
    pass1camera->setReadBuffer( GL_FRONT );
    pass1camera->setProjectionMatrixAsPerspective( osg::RadiansToDegrees( fovY ), ( width * 1.0 / height ), 0.1, 1000 );
    pass1camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // second pass

    _viewer->realize();

    // setup the callback to collect buffer frames
    _capture = new WindowCaptureScreen(gfxc, pass12tex);
    pass1camera->setFinalDrawCallback( _capture );

}

osg::ref_ptr<osg::Image> ImageViewerCaptureTool::grabImage() {

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
