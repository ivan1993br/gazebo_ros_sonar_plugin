#include <osgViewer/Viewer>

#include <osg/Texture>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/MatrixTransform>
#include <osg/Camera>
#include <osg/TexGenNode>
#include <osg/PositionAttitudeTransform>

#include <iostream>

#define BOOST_TEST_MODULE "DynamicCubeMap_test"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(DynamicCubeMap)

// create scene with multiple opaque objects
osg::ref_ptr<osg::Group> createScene()
{
    osg::ref_ptr<osg::Group> scene = new osg::Group;
    osg::ref_ptr<osg::Geode> geode_1 = new osg::Geode;
    scene->addChild(geode_1.get());

    osg::ref_ptr<osg::Geode> geode_2 = new osg::Geode;
    osg::ref_ptr<osg::MatrixTransform> transform_2 = new osg::MatrixTransform;
    transform_2->addChild(geode_2.get());
    transform_2->setUpdateCallback(new osg::AnimationPathCallback(osg::Vec3(0, 0, 0), osg::Y_AXIS, osg::inDegrees(45.0f)));
    scene->addChild(transform_2.get());

    const float radius = 0.8f;
    const float height = 1.0f;
    osg::ref_ptr<osg::ShapeDrawable> shape;

    // floor
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, -2.0f, 0.0f), 10, 0.1f, 10));
    shape->setColor(osg::Vec4(0.5f, 0.5f, 0.7f, 1.0f));
    geode_1->addDrawable(shape.get());

    // sphere
    shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(-3.0f, 0.0f, 0.0f), radius));
    shape->setColor(osg::Vec4(0.6f, 0.8f, 0.8f, 1.0f));
    geode_2->addDrawable(shape.get());

    // box
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(3.0f, 0.0f, 0.0f), 2 * radius));
    shape->setColor(osg::Vec4(0.4f, 0.9f, 0.3f, 1.0f));
    geode_2->addDrawable(shape.get());

    // cone
    shape = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0f, 0.0f, -3.0f), radius, height));
    shape->setColor(osg::Vec4(0.2f, 0.5f, 0.7f, 1.0f));
    geode_2->addDrawable(shape.get());

    // cylinder
    shape = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0.0f, 0.0f, 3.0f), radius, height));
    shape->setColor(osg::Vec4(1.0f, 0.3f, 0.3f, 1.0f));
    geode_2->addDrawable(shape.get());

    return scene;
}

// create a reflector sphere
osg::NodePath createReflector()
{
    osg::Geode* geode_1 = new osg::Geode;

    const float radius = 0.8f;
    osg::ref_ptr<osg::TessellationHints> hints = new osg::TessellationHints;
    hints->setDetailRatio(2.0f);
    osg::ShapeDrawable* shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), radius * 1.5f), hints.get());
    shape->setColor(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    geode_1->addDrawable(shape);

    osg::NodePath nodeList;
    nodeList.push_back(geode_1);

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

            for (size_t i = 0; i < 6 && i < _Cameras.size(); ++i) {
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

        osg::NodePath _reflectorNodePath;
        CameraList    _Cameras;
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
                osg::Quat quat = cv->getModelViewMatrix()->getRotate();
                _texmat->setMatrix(osg::Matrix::rotate(quat.inverse()));
            }
        }

    protected:

        osg::ref_ptr<osg::TexMat> _texmat;
};


osg::Group* createShadowedScene(osg::Node* reflectedSubgraph, osg::NodePath reflectorNodePath, unsigned int unit, const osg::Vec4& clearColor, unsigned tex_width, unsigned tex_height, osg::Camera::RenderTargetImplementation renderImplementation)
{
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
    for(size_t i = 0; i < 6; ++i) {
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
    {
        osg::Node* reflectorNode = reflectorNodePath.front();
        group->addChild(reflectorNode);

        osg::StateSet* stateset = reflectorNode->getOrCreateStateSet();
        stateset->setTextureAttributeAndModes(unit,texture,osg::StateAttribute::ON);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_S,osg::StateAttribute::ON);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_T,osg::StateAttribute::ON);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_R,osg::StateAttribute::ON);
        stateset->setTextureMode(unit,GL_TEXTURE_GEN_Q,osg::StateAttribute::ON);

        osg::TexMat* texmat = new osg::TexMat;
        stateset->setTextureAttributeAndModes(unit,texmat,osg::StateAttribute::ON);

        reflectorNode->setCullCallback(new TexMatCullCallback(texmat));
    }

    // add the reflector scene to draw just as normal
    group->addChild(reflectedSubgraph);

    // set an update callback to keep moving the camera and tex gen in the right direction.
    group->setUpdateCallback(new UpdateCameraAndTexGenCallback(reflectorNodePath, Cameras));

    return group;
}


BOOST_AUTO_TEST_CASE(dynamicCubeMapDemo_testCase) {

    osg::Camera::RenderTargetImplementation renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;

    osg::ref_ptr<osg::MatrixTransform> scene = new osg::MatrixTransform;
    scene->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(125.0),1.0,0.0,0.0));

    osg::ref_ptr<osg::Group> reflectedSubgraph = createScene();
    if (!reflectedSubgraph.valid()) exit(0);

    // construct the viewer.
    osgViewer::Viewer viewer;
    unsigned tex_width = 256;
    unsigned tex_height = 256;
    osg::ref_ptr<osg::Group> reflectedScene = createShadowedScene(reflectedSubgraph.get(),
                                                                  createReflector(),
                                                                  0,
                                                                  viewer.getCamera()->getClearColor(),
                                                                  tex_width,
                                                                  tex_height,
                                                                  renderImplementation);

    scene->addChild(reflectedScene.get());

    viewer.setSceneData(scene.get());

    viewer.run();
}

BOOST_AUTO_TEST_SUITE_END();
