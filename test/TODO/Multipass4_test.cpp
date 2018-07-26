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

#define BOOST_TEST_MODULE "Multipass3_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Multipass3)

static const char* mrtVertSource1 = {
    "#version 130\n"
    "out vec3 worldNormal;\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   worldNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n"
};

static const char* mrtFragSource1 = {
    "#version 130\n"
    "in vec3 worldNormal;\n"
    "void main(void)\n"
    "{\n"
    "   gl_FragData[0] = vec4(worldNormal, 0.0);\n"
    "}\n"
};

static const char* mrtVertSource2 = {
    "#version 130\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n"
};

static const char* mrtFragSource2 = {
    "#version 130\n"
    "uniform sampler2D pass12tex;\n"
    "in vec4 texcoord;\n"
    "void main(void)\n"
    "{\n"
    "   gl_FragData[0] = 1 - texture2D(pass12tex, gl_TexCoord[0].xy);\n"
    // "   gl_FragData[0] = 1 - texture2D(pass12tex, gl_FragCoord.xy / vec2(800.0,600.0));\n"
    "}\n"
};

struct SnapImage : public osg::Camera::DrawCallback {
    SnapImage(osg::GraphicsContext* gc, osg::Texture2D* tex ) {
        if (gc->getTraits()) {
            _texColor = tex;
        }
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const {
        // render texture to image
        renderInfo.getState()->applyTextureAttribute(0, _texColor);
        osg::ref_ptr<osg::Image> mColor = new osg::Image();
        mColor->readImageFromCurrentTexture(renderInfo.getContextID(), true, GL_UNSIGNED_BYTE);
        mColor->setInternalTextureFormat( GL_RGB );

        // save the rendered image to disk (for debug purposes)
        osgDB::ReaderWriter::WriteResult wrColor = osgDB::Registry::instance()->writeImage(*mColor, "./Test-color.png", NULL);
        if (!wrColor.success()) {
            osg::notify(osg::WARN) << "Color image: failed! (" << wrColor.message() << ")" << std::endl;
        }
    }
    osg::ref_ptr<osg::Texture2D> _texColor;
};

// create a RTT (render to texture) camera
osg::Camera* createRTTCamera( osg::Camera::BufferComponent buffer, int renderOrder, osg::Texture2D* tex, osg::GraphicsContext* gfxc, osg::Camera* cam = new osg::Camera )
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearColor( osg::Vec4() );
    camera->setClearMask( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setRenderOrder( osg::Camera::PRE_RENDER, renderOrder );
    camera->setViewport( 0, 0, tex->getTextureWidth(), tex->getTextureHeight() );
    camera->setGraphicsContext(gfxc);
    camera->setDrawBuffer( GL_FRONT );
    // camera->setReadBuffer( GL_FRONT );
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->attach( buffer, tex );
    return camera.release();
}

// // create the post render camera
// osg::Camera* createHUDCamera(osg::Camera::BufferComponent buffer, osg::Texture2D* tex, osg::GraphicsContext* gfxc, osg::Camera* cam = new osg::Camera)
// {
//     osg::ref_ptr<osg::Camera> camera = cam;
//     camera->setClearMask(0);
//     camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
//     camera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
//     camera->setRenderOrder( osg::Camera::POST_RENDER );
//     camera->setViewMatrix( osg::Matrixd::identity() );
//     camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
//     camera->setGraphicsContext(gfxc);
//     camera->setDrawBuffer( GL_FRONT );
//     camera->setReadBuffer( GL_FRONT );
//     camera->attach( buffer, tex );
//     return camera.release();
// }

// create float textures to be rendered in FBO
osg::Texture2D* createFloatTexture(uint w, uint h)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize( w, h );
    tex2D->setInternalFormat( GL_RGB );
    tex2D->setSourceFormat( GL_RGBA );
    tex2D->setSourceType( GL_UNSIGNED_BYTE );
    tex2D->setResizeNonPowerOfTwoHint( false );
    tex2D->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex2D->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    return tex2D.release();
}

BOOST_AUTO_TEST_CASE(multipass_testCase) {
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile("/home/romulo/Tools/OpenSceneGraph-Data/cow.osg");
    unsigned int w = 800, h = 600;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = w;
    traits->height = h;
    traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    // FIRST PASS: NORMAL DATA
    osg::ref_ptr<osg::Group> pass1root = new osg::Group();
    osg::ref_ptr<osg::StateSet> pass1state = pass1root->getOrCreateStateSet();
    osg::ref_ptr<osg::Program> pass1prog = new osg::Program();
    pass1prog->addShader( new osg::Shader(osg::Shader::VERTEX,   mrtVertSource1) );
    pass1prog->addShader( new osg::Shader(osg::Shader::FRAGMENT, mrtFragSource1) );
    pass1state->setAttributeAndModes( pass1prog.get(), osg::StateAttribute::ON );
    pass1root->addChild(scene);

    // first pass: texture, camera and scene
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::Texture2D* pass12tex = createFloatTexture(w, h);
    osg::ref_ptr<osg::Camera> pass1cam = createRTTCamera(osg::Camera::COLOR_BUFFER, 0, pass12tex, gfxc);
    pass1cam->addChild( pass1root );

    // first pass: shader
    osg::ref_ptr<osg::Group> pass2root = new osg::Group();
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    osg::ref_ptr<osg::Program> pass2prog = new osg::Program();
    pass2prog->addShader( new osg::Shader(osg::Shader::VERTEX,   mrtVertSource2) );
    pass2prog->addShader( new osg::Shader(osg::Shader::FRAGMENT, mrtFragSource2) );
    pass2state->setTextureAttributeAndModes(0, pass12tex, osg::StateAttribute::ON);
    pass2state->addUniform(new osg::Uniform("pass12tex", 0));
    pass2state->setAttributeAndModes( pass2prog.get(), osg::StateAttribute::ON );

    // // set RTT texture to quad
    // // osg::Geode* geode( new osg::Geode );
    // // geode->addDrawable( osg::createTexturedQuadGeometry( osg::Vec3(-1,-1,0), osg::Vec3(2.0,0.0,0.0), osg::Vec3(0.0,2.0,0.0)) );
    // // geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, pass12tex );
    // // geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    // // geode->getOrCreateStateSet()->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
    // // pass2root->addChild(geode);

    // final pass: texture and post render camera

    osg::Texture2D* pass2tex = createFloatTexture(w, h);
    osg::ref_ptr<osg::Camera> pass2cam = createRTTCamera(osg::Camera::COLOR_BUFFER, 1, pass2tex, gfxc);
    pass2cam->addChild( pass2root );

    // setup the root scene
    osg::ref_ptr<osg::Group> root = new osg::Group();
    root->addChild( pass2cam.get() );
    root->addChild( pass1cam.get() );
    //
    // // setup viewer
    // // viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    // viewer->getCamera()->setViewport(0,0,w,h);
    // // viewer->getCamera()->setGraphicsContext(gfxc);
    // // viewer->setUpViewInWindow( 0, 0, w, h );
    viewer->setSceneData( root );
    viewer->realize();

    // render texture to image
    SnapImage* finalDrawCallback = new SnapImage(gfxc, pass2tex);
    pass2cam->setFinalDrawCallback(finalDrawCallback);

    // osgDB::writeNodeFile(*root, "Test-color.osgt");

    viewer->run();
}

BOOST_AUTO_TEST_SUITE_END();
