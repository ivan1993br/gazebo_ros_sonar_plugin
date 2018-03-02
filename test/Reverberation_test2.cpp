// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/TriangleFunctor>

// C++ includes
#include <iostream>

#include <normal_depth_map/ImageViewerCaptureTool.hpp>
#include "TestHelper.hpp"

#define SHADER_PATH_FRAG "normal_depth_map/shaders/reverberation.frag"
#define SHADER_PATH_VERT "normal_depth_map/shaders/reverberation.vert"

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
            setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
        }

        void apply( osg::Geode& geode ) {
            for ( unsigned int i = 0; i<geode.getNumDrawables(); ++i ) {
                osg::TriangleFunctor<CollectTriangles> triangleCollector;

                geode.getDrawable(i)->accept(triangleCollector);
                std::cout << "Node: " << (i + 1) << ", Triangles: " << triangleCollector.verts->size() << std::endl;
                // for (unsigned int j = 0; j < triangleCollector.verts->size(); j++) {
                    // std::cout << "Index: " << i << "," << j << std::endl;
                    // osg::Matrix& matrix = _matrixStack.back();
                    // _vertices->push_back((*triangleCollector.verts)[j] * matrix);
               // }
            }
        }
};

osg::ref_ptr<osg::Group> createScene() {
    osg::ref_ptr<osg::Group> scene = new osg::Group;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    scene->addChild(geode.get());

    const float radius = 0.8f;
    const float height = 1.0f;
    osg::ref_ptr<osg::ShapeDrawable> shape;

    // sphere
    // shape = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(-3.0f, 0.0f, 0.0f), radius));
    // shape->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    // geode->addDrawable(shape.get());

    // // box
    shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 3.0f), 2* radius));
    shape->setColor(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    geode->addDrawable(shape.get());
    //
    // // cylinder
    // shape = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(3.0f, 0.0f, 0.0f), radius, height));
    // shape->setColor(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    // geode->addDrawable(shape.get());
    //
    // // cone
    // shape = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0f, 0.0f, -3.0f), radius, height));
    // shape->setColor(osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f));
    // geode->addDrawable(shape.get());

    return scene;
}

BOOST_AUTO_TEST_CASE(reverberation2_testCase) {

    osg::ref_ptr<osg::Group> scene = createScene();

    CollectTrianglesVisitor visitor;
    scene->accept(visitor);

    osgViewer::Viewer viewer;
    viewer.setSceneData(scene);
    viewer.setUpViewInWindow(0,0,600,600);
    viewer.run();
}
BOOST_AUTO_TEST_SUITE_END();
