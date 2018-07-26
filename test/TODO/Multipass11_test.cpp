// OSG includes
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/PolygonMode>

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
    "void main(void)\n"
    "{\n"
    // "   gl_FragData[0] = 1 - texture2D(pass12tex, gl_TexCoord[0].xy);\n"
    "   gl_FragData[0] = vec4(1,1,0,0);\n"
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
osg::Camera *createRTTCamera1(osg::Camera::BufferComponent buffer, osg::Texture2D *tex, osg::GraphicsContext *gfxc, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearColor(osg::Vec4());
    // camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER, 0);
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    // camera->setGraphicsContext(gfxc);
    // camera->setDrawBuffer(GL_FRONT);
    // camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->attach(buffer, tex);
    return camera.release();
}

osg::Camera *createRTTCamera2(osg::Camera::BufferComponent buffer, osg::Texture2D *tex, osg::GraphicsContext *gfxc, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearColor(osg::Vec4(0, 0, 0, 1));
    // camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER, 0);
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    // camera->setGraphicsContext(gfxc);
    // camera->setDrawBuffer(GL_FRONT);
    // camera->setReadBuffer(GL_FRONT);
    // camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->attach(buffer, tex);
    return camera.release();
}

// create the post render camera
osg::Camera *createHUDCamera(osg::Camera::BufferComponent buffer, osg::Texture2D *tex, osg::GraphicsContext *gfxc, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearMask(0);
    camera->setClearColor(osg::Vec4());
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setViewMatrix(osg::Matrixd::identity());
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    // camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER);
    // camera->setGraphicsContext(gfxc);
    // camera->setDrawBuffer(GL_FRONT);
    // camera->setReadBuffer(GL_FRONT);
    // camera->attach(buffer, tex);
    return camera.release();
}

// create the post render camera
osg::Camera *createHUDCamera(osg::Texture2D *tex, osg::Camera *cam = new osg::Camera)
{
    osg::ref_ptr<osg::Camera> camera = cam;
    camera->setClearMask(0);
    camera->setClearColor(osg::Vec4());
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setViewMatrix(osg::Matrixd::identity());
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    // camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER);
    // camera->setGraphicsContext(gfxc);
    // camera->setDrawBuffer(GL_FRONT);
    // camera->setReadBuffer(GL_FRONT);
    // camera->attach(buffer, tex);
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
    // tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
    // tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_BORDER);
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

osg::Camera *createHUDCamera(double left, double right, double bottom,
                             double top)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setAllowEventFocus(false);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(left, right,
                                                     bottom, top));
    camera->getOrCreateStateSet()->setMode(GL_LIGHTING,
                                           osg::StateAttribute::OFF);
    return camera.release();
}

osg::Geode *createScreenQuad(float width, float height, float scale = 1.0f)
{
    osg::Geometry *geom = osg::createTexturedQuadGeometry(
        osg::Vec3(), osg::Vec3(width, 0.0f, 0.0f),
        osg::Vec3(0.0f, height, 0.0f),
        0.0f, 0.0f, width * scale, height * scale);
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    quad->addDrawable(geom);

    int values =
        osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
    quad->getOrCreateStateSet()->setAttribute(
        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                             osg::PolygonMode::FILL),
        values);
    quad->getOrCreateStateSet()->setMode(GL_LIGHTING, values);
    return quad.release();
}

BOOST_AUTO_TEST_CASE(multipass_testCase)
{
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile("/home/romulo/Tools/OpenSceneGraph-Data/cow.osg");
    unsigned int w = 800, h = 600;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = w;
    traits->height = h;
    // traits->pbuffer = true;
    traits->readDISPLAY();
    osg::ref_ptr<osg::GraphicsContext> gfxc = osg::GraphicsContext::createGraphicsContext(traits.get());

    // get the main root node
    osg::ref_ptr<osg::Group> mainRoot = getMainGroup();
    osg::ref_ptr<osg::Group> pass1root = mainRoot->getChild(0)->asGroup();
    osg::ref_ptr<osg::Group> pass2root = mainRoot->getChild(1)->asGroup();

    // 1st pass: normal data
    osg::ref_ptr<osg::Texture2D> pass12tex = createFloatTexture(w, h);
    osg::ref_ptr<osg::Camera> pass1cam = createRTTCamera1(osg::Camera::COLOR_BUFFER, pass12tex, gfxc);
    pass1cam->addChild(scene);
    pass1cam->addChild(pass1root);

    // set the first pass texture as uniform of second pass
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    // pass2state->addUniform(new osg::Uniform(osg::Uniform::SAMPLER_2D, "pass12tex"));
    // pass2state->setTextureAttributeAndModes(0, pass12tex, osg::StateAttribute::ON);
    pass2state->addUniform(new osg::Uniform("pass12tex", 0));
    pass2state->setTextureAttributeAndModes(0, pass12tex, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // 2nd pass: inverted normal data
    osg::ref_ptr<osg::Texture2D> pass2tex = createFloatTexture(w, h);
    osg::ref_ptr<osg::Camera> pass2cam = createRTTCamera2(osg::Camera::COLOR_BUFFER, pass2tex, gfxc);
    pass2cam->addChild(scene);
    pass2cam->addChild(pass2root);


    // // set RTT texture to quad
    // osg::Geode *geode(new osg::Geode);
    // geode->addDrawable(osg::createTexturedQuadGeometry(osg::Vec3(-1, -1, 0), osg::Vec3(2.0, 0.0, 0.0), osg::Vec3(0.0, 2.0, 0.0)));
    // geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, pass2tex);
    // geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    // geode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    // pass2root->addChild(geode);

    // viewer camera
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Camera> hudCamera = createHUDCamera(0, 1, 0, 1);
    hudCamera->addChild(createScreenQuad(1.f, 1.f));
    osg::ref_ptr<osg::StateSet> mStateset = hudCamera->getOrCreateStateSet();
    mStateset->setTextureAttributeAndModes(0, pass2tex);

    // hudCamera->addChild(geode);

    // setup the root scene
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(pass1cam.get());
    root->addChild(pass2cam.get());
    root->addChild(hudCamera.get());

    // setup viewer
    // osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setUpViewInWindow(0, 0, w, h);
    viewer->setSceneData(root);
    viewer->realize();

    // render texture to image
    SnapImage *finalDrawCallback = new SnapImage(gfxc, pass2tex);
    pass2cam->setFinalDrawCallback(finalDrawCallback);
    viewer->run();
}

BOOST_AUTO_TEST_SUITE_END();
