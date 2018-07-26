// OSG includes
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>

// C++ includes
#include <string>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Multipass_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Multipass)

static const char *mrtVertSource1 = {
    "#version 130\n"
    "out vec3 worldNormal;\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   worldNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n"};

static const char *mrtFragSource1 = {
    "#version 130\n"
    "in vec3 worldNormal;\n"
    "void main(void)\n"
    "{\n"
    "   gl_FragData[0] = vec4(worldNormal, 0.0);\n"
    "}\n"};

static const char *mrtVertSource2 = {
    "#version 130\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n"};

static const char *mrtFragSource2 = {
    "#version 130\n"
    "uniform sampler2D pass12tex;\n"
    "uniform float myUniform;\n"
    "in vec4 texcoord;\n"
    "void main(void)\n"
    "{\n"
    // "   gl_FragData[0] = 1 - texture2D(pass12tex, gl_TexCoord[0].xy);\n"
    "   gl_FragData[0] = 1 - texture2D(pass12tex, gl_FragCoord.xy / vec2(800.0,600.0));\n"
    // "   gl_FragData[0] = vec4(1,1,0,0);\n"
    // "   if (myUniform == 0.75) gl_FragData[0] = vec4(1,1,0,0);\n"
    // "   else gl_FragData[0] = vec4(1,0,1,1);\n"
    "}\n"};

struct SnapImage : public osg::Camera::DrawCallback
{
    SnapImage(osg::GraphicsContext *gc, osg::Texture2D *tex)
    {
        if (gc->getTraits())
        {
            _texColor = tex;
        }
    }

    virtual void operator()(osg::RenderInfo &renderInfo) const
    {
        // render texture to image
        renderInfo.getState()->applyTextureAttribute(0, _texColor);
        osg::ref_ptr<osg::Image> mColor = new osg::Image();
        mColor->readImageFromCurrentTexture(renderInfo.getContextID(), true, GL_UNSIGNED_BYTE);
        mColor->setInternalTextureFormat(GL_RGB);

        // save the rendered image to disk (for debug purposes)
        osgDB::ReaderWriter::WriteResult wrColor = osgDB::Registry::instance()->writeImage(*mColor, "./Test-color.png", NULL);
        if (!wrColor.success())
        {
            osg::notify(osg::WARN) << "Color image: failed! (" << wrColor.message() << ")" << std::endl;
        }
    }
    osg::ref_ptr<osg::Texture2D> _texColor;
};

// create a RTT (render to texture) camera
osg::Camera *createRTTCamera(osg::Camera::BufferComponent buffer, int index, osg::Texture2D *tex, osg::GraphicsContext *gfxc, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearColor(osg::Vec4());
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER, index);
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    camera->setGraphicsContext(gfxc);
    camera->setDrawBuffer(GL_FRONT);
    // camera->setReadBuffer( GL_FRONT );
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->attach(buffer, tex);
    return camera.release();
}

// create float textures to be rendered in FBO
osg::Texture2D *createFloatTexture(uint w, uint h)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize(w, h);
    tex2D->setInternalFormat(GL_RGB);
    tex2D->setSourceFormat(GL_RGBA);
    tex2D->setSourceType(GL_UNSIGNED_BYTE);
    tex2D->setResizeNonPowerOfTwoHint(false);
    tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    return tex2D.release();
}

osg::ref_ptr<osg::Group> getMainGroup()
{
    osg::ref_ptr<osg::Group> localRoot = new osg::Group();

    // 1st pass: primary reflections by rasterization pipeline
    osg::ref_ptr<osg::Group> pass1root = new osg::Group();
    osg::ref_ptr<osg::Program> pass1prog = new osg::Program();
    osg::ref_ptr<osg::StateSet> pass1state = pass1root->getOrCreateStateSet();
    pass1prog->addShader(new osg::Shader(osg::Shader::VERTEX, mrtVertSource1));
    pass1prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, mrtFragSource1));
    pass1state->setAttributeAndModes(pass1prog, osg::StateAttribute::ON);

    // 2nd pass: secondary reflections by ray-triangle intersection
    osg::ref_ptr<osg::Group> pass2root = new osg::Group();
    osg::ref_ptr<osg::Program> pass2prog = new osg::Program();
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    pass2prog->addShader(new osg::Shader(osg::Shader::VERTEX, mrtVertSource2));
    pass2prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, mrtFragSource2));
    pass2state->setAttributeAndModes(pass2prog, osg::StateAttribute::ON);

    // set the main root
    localRoot->addChild(pass1root);
    localRoot->addChild(pass2root);
    localRoot->addChild(osgDB::readNodeFile("/home/romulo/Tools/OpenSceneGraph-Data/cow.osg"));

    return localRoot;
}

// create the post render camera
osg::Camera *createHUDCamera(osg::Camera::BufferComponent buffer, osg::Texture2D *tex, osg::GraphicsContext *gfxc, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearMask(0);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER);
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setViewMatrix(osg::Matrixd::identity());
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    camera->setGraphicsContext(gfxc);
    camera->setDrawBuffer(GL_FRONT);
    camera->setReadBuffer(GL_FRONT);
    camera->attach(buffer, tex);
    return camera.release();
}

// BOOST_AUTO_TEST_CASE(multipass_testCase)
// {
//     unsigned int w = 800, h = 600;

//     osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
//     traits->width = w;
//     traits->height = h;
//     traits->pbuffer = true;
//     traits->readDISPLAY();
//     osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

//     osg::ref_ptr<osg::Group> mainRoot = getMainGroup();
//     osg::ref_ptr<osg::Group> pass1root = mainRoot->getChild(0)->asGroup();
//     osg::ref_ptr<osg::Group> pass2root = mainRoot->getChild(1)->asGroup();
//     for (size_t i = 2; i < mainRoot->getNumChildren(); i++)
//     {
//         pass1root->addChild(mainRoot->getChild(i));
//         pass2root->addChild(mainRoot->getChild(i));
//     }

//     // first pass: texture, camera and scene
//     osg::ref_ptr<osg::Texture2D> pass12tex = createFloatTexture(w, h);
//     osg::ref_ptr<osg::Camera> pass1cam = createRTTCamera(osg::Camera::COLOR_BUFFER, 0, pass12tex, gfxc);
//     pass1cam->addChild(pass1root);

//     // set RTT texture to quad
//     osg::Geode *geode(new osg::Geode);
//     geode->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-1, -1, 0), osg::Vec3(2.0, 0.0, 0.0), osg::Vec3(0.0, 2.0, 0.0)));
//     geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, pass12tex);
//     geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
//     geode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
//     pass2root->addChild(geode);

//     // set the first pass texture as uniform of second pass
//     osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
//     pass2state->addUniform(new osg::Uniform("pass12tex", 0));
//     pass2state->setTextureAttributeAndModes(0, pass12tex, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

//     // second pass: texture, camera and scene
//     osg::ref_ptr<osg::Texture2D> pass2tex = createFloatTexture(w, h);
//     osg::ref_ptr<osg::Camera> pass2cam = createHUDCamera(osg::Camera::COLOR_BUFFER, pass2tex, gfxc);
//     pass2cam->addChild(pass2root);

//     // setup viewer
//     osg::ref_ptr<osg::Group> root = new osg::Group;
//     root->addChild(pass1cam.get());
//     root->addChild(pass2cam.get());

//     osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
//     // viewer->setUpViewInWindow(0, 0, w, h);
//     viewer->getCamera()->setViewport(0, 0, w, h);
//     viewer->setSceneData(root.get());
//     viewer->realize();

//     // render texture to image
//     SnapImage *finalDrawCallback = new SnapImage(gfxc, pass2tex);
//     pass2cam->setFinalDrawCallback(finalDrawCallback);
//     viewer->run();
// }

BOOST_AUTO_TEST_CASE(multipass_testCase)
{
    unsigned int w = 800, h = 600;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = w;
    traits->height = h;
    traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    osg::ref_ptr<osg::Group> mainRoot = getMainGroup();
    osg::ref_ptr<osg::Group> pass1root = mainRoot->getChild(0)->asGroup();
    osg::ref_ptr<osg::Group> pass2root = mainRoot->getChild(1)->asGroup();
    for (size_t i = 2; i < mainRoot->getNumChildren(); i++)
    {
        pass1root->addChild(mainRoot->getChild(i));
        pass2root->addChild(mainRoot->getChild(i));
    }

    // first pass: texture, camera and scene
    osg::ref_ptr<osg::Texture2D> pass12tex = createFloatTexture(w, h);
    osg::ref_ptr<osg::Camera> pass1cam = createRTTCamera(osg::Camera::COLOR_BUFFER, 0, pass12tex, gfxc);
    pass1cam->addChild(pass1root);

    // set RTT texture to quad
    // osg::Geode *geode(new osg::Geode);
    // geode->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-1, -1, 0), osg::Vec3(2.0, 0.0, 0.0), osg::Vec3(0.0, 2.0, 0.0)));
    // geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, pass12tex);
    // geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    // geode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    // pass2root->addChild(geode);

    // set the first pass texture as uniform of second pass
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    pass2state->addUniform(new osg::Uniform("pass12tex", 0));
    pass2state->setTextureAttributeAndModes(0, pass12tex, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // second pass: texture, camera and scene
    osg::ref_ptr<osg::Texture2D> pass2tex = createFloatTexture(w, h);
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osg::Camera> pass2cam = createHUDCamera(osg::Camera::COLOR_BUFFER, pass2tex, gfxc);
    pass2cam->addChild(pass2root);

    // setup viewer
    osg::ref_ptr<osg::Group> root = new osg::Group;
    // root->addChild(pass1cam.get());
    // root->addChild(pass2cam.get());
    root->addChild(pass2root.get());

    // viewer->setUpViewInWindow(0, 0, w, h);
    viewer->getCamera()->setViewport(0, 0, w, h);
    // viewer.setSceneData(root.get());
    viewer->setSceneData(root.get());
    viewer->realize();

    // render texture to image
    SnapImage *finalDrawCallback = new SnapImage(gfxc, pass2tex);
    pass2cam->setFinalDrawCallback(finalDrawCallback);
    viewer->run();
}

BOOST_AUTO_TEST_SUITE_END();
