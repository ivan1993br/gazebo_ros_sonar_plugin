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

#define SHADER_PATH_FRAG "normal_depth_map/shaders/normalDepthMap.frag"
#define SHADER_PATH_VERT "normal_depth_map/shaders/normalDepthMap.vert"

#define BOOST_TEST_MODULE "DynamicCubeMap_test"
#include <boost/test/unit_test.hpp>

using namespace osg;
using namespace normal_depth_map;
using namespace test_helper;

unsigned int numTextures = 6;

enum TextureUnitTypes {
    TEXTURE_UNIT_DIFFUSE,
    TEXTURE_UNIT_NORMAL,
    TEXTURE_UNIT_CUBEMAP
};

BOOST_AUTO_TEST_SUITE(DynamicCubeMap2)

cv::Mat setChannelValue(cv::Mat img, int channel, uchar value) {
    if (channel < 0 || channel > 2)
        std::runtime_error("Channel number must be between 1 and 3");

    std::vector<cv::Mat> channels;
    cv::split(img, channels);
    channels[channel].setTo(value);
    cv::Mat out;
    cv::merge(channels, out);

    return out;
}

osg::ref_ptr<osg::Group> _create_scene() {
    osg::ref_ptr<osg::Group> scene = new osg::Group;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    scene->addChild(geode.get());

    const float radius = 0.8f;
    const float height = 1.0f;
    osg::ref_ptr<osg::ShapeDrawable> shape;

    // sphere
    shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(-3.0f, 0.0f, 0.0f), radius));
    shape->setColor(osg::Vec4(0.6f, 0.8f, 0.8f, 1.0f));
    geode->addDrawable(shape.get());

    // box
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(3.0f, 0.0f, 0.0f), 2 * radius));
    shape->setColor(osg::Vec4(0.4f, 0.9f, 0.3f, 1.0f));
    geode->addDrawable(shape.get());

    // cone
    shape = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0.0f, 0.0f, -3.0f), radius, height));
    shape->setColor(osg::Vec4(1.0f, 0.3f, 0.3f, 1.0f));
    geode->addDrawable(shape.get());

    // cylinder
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 3.0f), 2* radius));
    shape->setColor(osg::Vec4(0.8f, 0.8f, 0.4f, 1.0f));
    geode->addDrawable(shape.get());

    return scene;
}

osg::NodePath createReflector() {
    Geode* node = new Geode;
    const float radius = 0.8f;
    ref_ptr<TessellationHints> hints = new TessellationHints;
    hints->setDetailRatio(2.0f);
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

class TexMatCullCallback : public osg::NodeCallback
{
    public:

        TexMatCullCallback(osg::TexMat* texmat):
            _texmat(texmat)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            // first update subgraph to make sure objects are all moved into position
            traverse(node,nv);

            osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
            if (cv)
            {
				osg::Quat q = osg::Matrix::inverse(*cv->getModelViewMatrix()).getRotate();


				float yaw2 = asin(-2.0f*(q.x()*q.z() - q.w()*q.y()));
				osg::Matrixd mxY;
				mxY.makeRotate(yaw2,osg::Vec3(0,0,1));

				osg::Matrixd mx = mxY;
                _texmat->setMatrix(mx);
            }
        }

    protected:

        osg::ref_ptr<TexMat>    _texmat;
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

osg::Group* createShadowedScene(osg::Node* reflectedSubgraph, osg::NodePath reflectorNodePath, unsigned int unit, const osg::Vec4& clearColor, unsigned tex_width, unsigned tex_height, osg::Camera::RenderTargetImplementation renderImplementation, osg::Camera* camera = 0) {
    osg::Group* group = new osg::Group;

    osg::TextureCubeMap* texture = new osg::TextureCubeMap;
    texture->setTextureSize(tex_width, tex_height);

    texture->setInternalFormat(GL_RGB);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    texture->setFilter(osg::TextureCubeMap::MIN_FILTER,osg::TextureCubeMap::LINEAR);
    texture->setFilter(osg::TextureCubeMap::MAG_FILTER,osg::TextureCubeMap::LINEAR);

    // set up the render to texture cameras.
    UpdateCameraAndTexGenCallback::CameraList Cameras;
    for(unsigned int i = 0; i < 6; ++i) {
        // create the camera
        osg::Camera* camera = new osg::Camera;

        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setClearColor(clearColor);

        // set viewport
        camera->setViewport(0,0,tex_width,tex_height);

        // set the camera to render before the main camera.
        camera->setRenderOrder(osg::Camera::PRE_RENDER);

        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation(renderImplementation);

        // attach the texture and use it as the color buffer.
        camera->attach(osg::Camera::COLOR_BUFFER, texture, 0, i);

        // add subgraph to render
        camera->addChild(reflectedSubgraph);

        group->addChild(camera);

        Cameras.push_back(camera);
    }

    // create the texgen node to project the tex coords onto the subgraph
    osg::TexGenNode* texgenNode = new osg::TexGenNode;
    texgenNode->getTexGen()->setMode(osg::TexGen::REFLECTION_MAP);
    texgenNode->setTextureUnit(unit);
    group->addChild(texgenNode);

    // set the reflected subgraph so that it uses the texture and tex gen settings.
	osg::Node* reflectorNode = reflectorNodePath.front();
    {

        group->addChild(reflectorNode);

        osg::StateSet* stateset = reflectorNode->getOrCreateStateSet();
		stateset->setTextureAttributeAndModes(unit,texture,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_S,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_T,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_R,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_Q,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        osg::TexMat* texmat = new osg::TexMat;
        stateset->setTextureAttributeAndModes(unit,texmat,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        reflectorNode->setCullCallback(new TexMatCullCallback(texmat));
    }

	osg::StateSet* ss = reflectorNode->getOrCreateStateSet();
	ss->setTextureAttributeAndModes(unit,texture,osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

	osg::Program* program = new osg::Program;
    osg::ref_ptr<osg::Shader> shaderVertex = osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(SHADER_PATH_VERT));
    osg::ref_ptr<osg::Shader> shaderFragment = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(SHADER_PATH_FRAG));
    program->addShader(shaderFragment);
    program->addShader(shaderVertex);
    ss->setAttributeAndModes( program, osg::StateAttribute::ON );
	ss->addUniform( new osg::Uniform("cubeMap", unit) );

	osg::Uniform* u = new osg::Uniform("cameraPos",osg::Vec3());
	u->setUpdateCallback( new UpdateCameraPosUniformCallback( camera ) );
	ss->addUniform( u );

    // add the reflector scene to draw just as normal
    group->addChild(reflectedSubgraph);

    // set an update callback to keep moving the camera and tex gen in the right direction.
    group->setUpdateCallback(new UpdateCameraAndTexGenCallback(reflectorNodePath, Cameras));

    return group;
}


// int main(int argc, char** argv){}
BOOST_AUTO_TEST_CASE(dynamicCubeMap2Demo_testCase) {
    // construct the viewer.
    osgViewer::Viewer viewer;

    unsigned tex_width = 256;
    unsigned tex_height = 256;

    osg::Camera::RenderTargetImplementation renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;

    osg::ref_ptr<osg::Group> scene = new osg::Group;
    osg::ref_ptr<osg::Group> reflectedSubgraph = _create_scene();
    if (!reflectedSubgraph.valid()) exit(0);

    osg::ref_ptr<osg::Group> reflectedScene = createShadowedScene(
			reflectedSubgraph.get(),
			createReflector(),
			TEXTURE_UNIT_CUBEMAP,
			viewer.getCamera()->getClearColor(),
            tex_width,
			tex_height,
			renderImplementation,
			viewer.getCamera());

    scene->addChild(reflectedScene.get());
    viewer.setSceneData(scene.get());
    viewer.setUpViewInWindow(0,0,600,600);
    viewer.run();

    // osg::Vec3d eye, center, up;
    // viewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
    // std::cout << "eye    : " << eye.x() << "," << eye.y() << "," << eye.z() << std::endl;
    // std::cout << "center : " << center.x() << "," << center.y() << "," << center.z() << std::endl;
    // std::cout << "up     : " << up.x() << "," << up.y() << "," << up.z() << std::endl;

    // set view direction
    // osg::Vec3d eye(0, -18.0198, 1.25);
    // osg::Vec3d center(0, -17.0198, 1.25);
    // osg::Vec3d up(0, 0, 1);
    //
    // float maxRange = 25.0f;
    // float fovX = M_PI / 3;  // 60 degrees
    // float fovY = M_PI / 3;  // 60 degrees

    // // results
    // cv::Mat cvShader = computeNormalDepthMap(scene.get(), maxRange, fovX, fovY, 0, eye, center, up, 500);
    // cv::Mat cvDepth = setChannelValue(cvShader, 0, 0);
    // cv::Mat cvNormal = setChannelValue(cvShader, 1, 0);
    //
    // // output
    // cv::imshow("cvShader", cvShader);
    // cv::imshow("cvDepth", cvDepth);
    // cv::imshow("cvNormal", cvNormal);
    // cv::waitKey();
}
BOOST_AUTO_TEST_SUITE_END();
