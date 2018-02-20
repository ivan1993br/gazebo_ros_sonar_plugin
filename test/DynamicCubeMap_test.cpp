// OSG includes
#include <osgViewer/Viewer>
#include <osg/Texture>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Camera>
#include <osg/TexGenNode>
#include <osgDB/FileUtils>

// C++ includes
#include <iostream>

// Rock includes
#include <normal_depth_map/NormalDepthMap.hpp>
#include <normal_depth_map/ImageViewerCaptureTool.hpp>
#include "TestHelper.hpp"

#define SHADER_PATH_FRAG "normal_depth_map/shaders/temp/cubemap.frag"
#define SHADER_PATH_VERT "normal_depth_map/shaders/temp/cubemap.vert"

#define BOOST_TEST_MODULE "DynamicCubeMap_test"
#include <boost/test/unit_test.hpp>

using namespace osg;
using namespace normal_depth_map;
using namespace test_helper;

static const int numTextures = 6;

enum TextureUnitTypes {
    TEXTURE_UNIT_DIFFUSE,
    TEXTURE_UNIT_NORMAL,
    TEXTURE_UNIT_CUBEMAP
};

BOOST_AUTO_TEST_SUITE(DynamicCubeMap)

osg::ref_ptr<osg::Group> createScene() {
    osg::ref_ptr<osg::Group> scene = new osg::Group;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    scene->addChild(geode.get());

    const float radius = 0.8f;
    const float height = 1.0f;
    osg::ref_ptr<osg::ShapeDrawable> shape;

    // sphere
    shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(-3.0f, 0.0f, 0.0f), radius));
    shape->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    geode->addDrawable(shape.get());

    // box
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 3.0f), 2* radius));
    shape->setColor(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    geode->addDrawable(shape.get());

    // cylinder
    shape = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(3.0f, 0.0f, 0.0f), radius, height));
    shape->setColor(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    geode->addDrawable(shape.get());

    // cone
    shape = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0f, 0.0f, -3.0f), radius, height));
    shape->setColor(osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f));
    geode->addDrawable(shape.get());

    return scene;
}

osg::NodePath createReflector() {
    Geode* node = new Geode;
    const float radius = 0.8f;
    ref_ptr<TessellationHints> hints = new TessellationHints;
    hints->setDetailRatio(10.0f);
    ShapeDrawable* shape = new ShapeDrawable(new Sphere(Vec3(0.0f, 0.0f, 0.0f), radius * 1.5f), hints.get());
    shape->setColor(Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    node->addDrawable(shape);

    osg::NodePath nodeList;
    nodeList.push_back(node);

    return nodeList;
}

class UpdateCameraAndTexGenCallback : public osg::NodeCallback
{
    public:

        typedef std::vector< osg::ref_ptr<osg::Camera> >  CameraList;

        UpdateCameraAndTexGenCallback(osg::NodePath& reflectorNodePath, CameraList& Cameras):
            _reflectorNodePath(reflectorNodePath),
            _Cameras(Cameras)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            // first update subgraph to make sure objects are all moved into position
            traverse(node,nv);

            // compute the position of the center of the reflector subgraph
            osg::Matrixd worldToLocal = osg::computeWorldToLocal(_reflectorNodePath);
            osg::BoundingSphere bs = _reflectorNodePath.back()->getBound();
            osg::Vec3 position = bs.center();

            typedef std::pair<osg::Vec3, osg::Vec3> ImageData;
            const ImageData id[] =
            {
                ImageData( osg::Vec3( 1,  0,  0), osg::Vec3( 0, -1,  0) ), // +X
                ImageData( osg::Vec3(-1,  0,  0), osg::Vec3( 0, -1,  0) ), // -X
                ImageData( osg::Vec3( 0,  1,  0), osg::Vec3( 0,  0,  1) ), // +Y
                ImageData( osg::Vec3( 0, -1,  0), osg::Vec3( 0,  0, -1) ), // -Y
                ImageData( osg::Vec3( 0,  0,  1), osg::Vec3( 0, -1,  0) ), // +Z
                ImageData( osg::Vec3( 0,  0, -1), osg::Vec3( 0, -1,  0) )  // -Z
            };

            for(unsigned int i = 0; i < 6 && i < _Cameras.size(); ++i) {
                osg::Matrix localOffset;
                localOffset.makeLookAt(position,position+id[i].first,id[i].second);

                osg::Matrix viewMatrix = worldToLocal*localOffset;

                _Cameras[i]->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
                _Cameras[i]->setProjectionMatrixAsFrustum(-1.0,1.0,-1.0,1.0,1.0,10000.0);
                _Cameras[i]->setViewMatrix(viewMatrix);
            }
        }

    protected:

        virtual ~UpdateCameraAndTexGenCallback() {}

        osg::NodePath               _reflectorNodePath;
        CameraList                  _Cameras;
};

class UpdateCameraPosUniformCallback : public osg::Uniform::Callback
{
public:
	UpdateCameraPosUniformCallback(osg::Camera* camera)
		: mCamera(camera)
	{
	}

	virtual void operator () (osg::Uniform* u, osg::NodeVisitor*)
	{
		osg::Vec3 eye;
		osg::Vec3 center;
		osg::Vec3 up;
		mCamera->getViewMatrixAsLookAt(eye,center,up);

		u->set(eye);
	}
protected:
	osg::Camera* mCamera;
};

osg::TextureCubeMap* createRenderTexture(int tex_width, int tex_height) {
    // create simple 2D texture
    osg::ref_ptr<osg::TextureCubeMap> texture = new osg::TextureCubeMap;
    texture->setTextureSize(tex_width, tex_height);
    texture->setFilter(osg::TextureCubeMap::MIN_FILTER,osg::TextureCubeMap::LINEAR);
    texture->setFilter(osg::TextureCubeMap::MAG_FILTER,osg::TextureCubeMap::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    texture->setInternalFormat(GL_RGBA32F);
    texture->setSourceFormat(GL_RGBA);
    texture->setSourceType(GL_FLOAT);

    return texture.release();
}

osg::TextureCubeMap* createDepthTexture(int tex_width, int tex_height) {
    // create simple 2D texture
    osg::ref_ptr<osg::TextureCubeMap> texture = new osg::TextureCubeMap;
    texture->setTextureSize(tex_width, tex_height);
    texture->setFilter(osg::TextureCubeMap::MIN_FILTER,osg::TextureCubeMap::LINEAR);
    texture->setFilter(osg::TextureCubeMap::MAG_FILTER,osg::TextureCubeMap::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    texture->setInternalFormat(GL_DEPTH_COMPONENT32F);
    texture->setSourceFormat(GL_DEPTH_COMPONENT);
    texture->setSourceType(GL_FLOAT);

    return texture.release();
}

osg::Camera* createRTTCamera( osg::Camera::BufferComponent buffer, osg::TextureCubeMap* tex, unsigned int face = 0, unsigned int level = 0) {
    // set the viewport size and clean the background color and buffer
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    camera->setClearColor(osg::Vec4());
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // force the camera to be rendered before the main scene, and use the RTT technique with FBO
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->attach(buffer, tex, level, face);

    return camera.release();
}

BOOST_AUTO_TEST_CASE(cubemapColorBuffer_demo) {
    // construct the viewer.
    osgViewer::Viewer viewer;

    unsigned tex_width = 256;
    unsigned tex_height = 256;

    osg::ref_ptr<osg::Group> scene = new osg::Group;
    osg::ref_ptr<osg::Group> reflectedSubgraph = createScene();
    if (!reflectedSubgraph.valid()) exit(0);

    osg::NodePath reflectorNodePath = createReflector();

    osg::Group* group = new osg::Group;

    osg::TextureCubeMap* texture = createRenderTexture(tex_width, tex_height);

    // set up the render to texture cameras.
    UpdateCameraAndTexGenCallback::CameraList Cameras;

    for(int i = 0; i < numTextures; ++i) {
        osg::Camera* camera = createRTTCamera(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + i), texture, i);

        // add subgraph to render
        camera->addChild(reflectedSubgraph);

        group->addChild(camera);

        Cameras.push_back(camera);
    }

    // create the texgen node to project the tex coords onto the subgraph
    osg::TexGenNode* texgenNode = new osg::TexGenNode;
    texgenNode->getTexGen()->setMode(osg::TexGen::REFLECTION_MAP);
    texgenNode->setTextureUnit(0);
    group->addChild(texgenNode);

    // set the reflected subgraph so that it uses the texture and tex gen settings.
    osg::Node* reflectorNode = reflectorNodePath.front();
    group->addChild(reflectorNode);

    osg::StateSet* ss = reflectorNode->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    osg::Program* program = new osg::Program;
    osg::ref_ptr<osg::Shader> shaderVertex = osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(SHADER_PATH_VERT));
    osg::ref_ptr<osg::Shader> shaderFragment = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(SHADER_PATH_FRAG));
    program->addShader(shaderFragment);
    program->addShader(shaderVertex);
    ss->setAttributeAndModes( program, osg::StateAttribute::ON );

    ss->addUniform( new osg::Uniform("cubemapTexture", 0) );
    ss->addUniform( new osg::Uniform("colorBuffer", true));

    osg::Uniform* u = new osg::Uniform("cameraPos",osg::Vec3());
    u->setUpdateCallback( new UpdateCameraPosUniformCallback( viewer.getCamera() ) );
    ss->addUniform( u );

    // add the reflector scene to draw just as normal
    group->addChild(reflectedSubgraph);

    // set an update callback to keep moving the camera and tex gen in the right direction.
    group->setUpdateCallback(new UpdateCameraAndTexGenCallback(reflectorNodePath, Cameras));

    scene->addChild(group);

    viewer.setSceneData(scene.get());
    viewer.setUpViewInWindow(0,0,600,600);
    viewer.run();
}

BOOST_AUTO_TEST_CASE(cubemapDepthBuffer_demo) {
    // construct the viewer.
    osgViewer::Viewer viewer;

    unsigned tex_width = 256;
    unsigned tex_height = 256;

    osg::ref_ptr<osg::Group> scene = new osg::Group;
    osg::ref_ptr<osg::Group> reflectedSubgraph = createScene();
    if (!reflectedSubgraph.valid()) exit(0);

    osg::NodePath reflectorNodePath = createReflector();

    osg::Group* group = new osg::Group;

    osg::TextureCubeMap* texture = createDepthTexture(tex_width, tex_height);

    // set up the render to texture cameras.
    UpdateCameraAndTexGenCallback::CameraList Cameras;

    for(int i = 0; i < numTextures; ++i) {
        osg::Camera* camera = createRTTCamera(osg::Camera::BufferComponent(osg::Camera::DEPTH_BUFFER), texture, i);

        // add subgraph to render
        camera->addChild(reflectedSubgraph);

        group->addChild(camera);

        Cameras.push_back(camera);
    }

    // create the texgen node to project the tex coords onto the subgraph
    osg::TexGenNode* texgenNode = new osg::TexGenNode;
    texgenNode->getTexGen()->setMode(osg::TexGen::REFLECTION_MAP);
    texgenNode->setTextureUnit(0);
    group->addChild(texgenNode);

    // set the reflected subgraph so that it uses the texture and tex gen settings.
    osg::Node* reflectorNode = reflectorNodePath.front();
    group->addChild(reflectorNode);

    osg::StateSet* ss = reflectorNode->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    osg::Program* program = new osg::Program;
    osg::ref_ptr<osg::Shader> shaderVertex = osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(SHADER_PATH_VERT));
    osg::ref_ptr<osg::Shader> shaderFragment = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(SHADER_PATH_FRAG));
    program->addShader(shaderFragment);
    program->addShader(shaderVertex);
    ss->setAttributeAndModes( program, osg::StateAttribute::ON );

    ss->addUniform( new osg::Uniform("cubemapTexture", 0) );
    ss->addUniform( new osg::Uniform("colorBuffer", false));

    osg::Uniform* u = new osg::Uniform("cameraPos",osg::Vec3());
    u->setUpdateCallback( new UpdateCameraPosUniformCallback( viewer.getCamera() ) );
    ss->addUniform( u );

    // add the reflector scene to draw just as normal
    group->addChild(reflectedSubgraph);

    // set an update callback to keep moving the camera and tex gen in the right direction.
    group->setUpdateCallback(new UpdateCameraAndTexGenCallback(reflectorNodePath, Cameras));

    scene->addChild(group);

    viewer.setSceneData(scene.get());
    viewer.setUpViewInWindow(0,0,600,600);
    viewer.run();
}
BOOST_AUTO_TEST_SUITE_END();
