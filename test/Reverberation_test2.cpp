// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TriangleFunctor>

// C++ includes
#include <iostream>

#include <normal_depth_map/ImageViewerCaptureTool.hpp>
#include "TestHelper.hpp"

// #define SHADER_PATH_FRAG "normal_depth_map/shaders/reverberation.frag"
// #define SHADER_PATH_VERT "normal_depth_map/shaders/reverberation.vert"

#define BOOST_TEST_MODULE "DynamicCubeMap_test"
#include <boost/test/unit_test.hpp>

using namespace osg;
using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Reverberation2)

struct CollectTriangles {
    CollectTriangles() {
        verts = new osg::Vec3Array();
    }

    inline void operator () (const osg::Vec3& v1,const osg::Vec3& v2,const osg::Vec3& v3, bool treatVertexDataAsTemporary) {
        verts->push_back(v1);
        verts->push_back(v2);
        verts->push_back(v3);
    }

    osg::ref_ptr< osg::Vec3Array > verts;
};


class CollectTrianglesVisitor : public osg::NodeVisitor {

    public:
        CollectTrianglesVisitor() {
            vertices = new osg::Vec3Array();
            setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
        }

        void apply( osg::Geode& geode ) {
            for ( unsigned int i = 0; i<geode.getNumDrawables(); ++i ) {
                osg::TriangleFunctor<CollectTriangles> triangleCollector;
                geode.getDrawable(i)->accept(triangleCollector);
                vertices->insert(vertices->end(), triangleCollector.verts->begin(), triangleCollector.verts->end());
                // std::cout << "Node: " << i << " / Vertices: " << triangleCollector.verts->size() << std::endl;
            }
        }
        osg::ref_ptr< osg::Vec3Array > vertices;
};

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

BOOST_AUTO_TEST_CASE(reverberation2_testCase) {
    // create scene with models
    osg::ref_ptr<osg::Group> scene = createScene();

    // get all triangles' vertices from all scene models
    CollectTrianglesVisitor visitor;
    scene->accept(visitor);

    // until OpenGL 4.3, arrays in GLSL must be fixed, compile-time size.
    // In this case, the triangles' vertices are stored as texture and passed to shader.
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(visitor.vertices->size(), 1, 1, GL_RGBA8, GL_RGBA, GL_FLOAT, (unsigned char*) &visitor.vertices[0], osg::Image::NO_DELETE);
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage(image);

    // TODO: pass the texture to GLSL as uniform

    // display the scene
    osgViewer::Viewer viewer;
    viewer.setSceneData(scene);
    viewer.setUpViewInWindow(0,0,600,600);
    viewer.run();
}
BOOST_AUTO_TEST_SUITE_END();
