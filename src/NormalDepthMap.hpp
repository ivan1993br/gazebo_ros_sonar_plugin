/*
 * NormalDepthMap.hpp
 *
 *  Created on: Mar 27, 2015
 *      Author: tiagotrocoli
 */

#ifndef SIMULATION_NORMAL_DEPTH_MAP_SRC_NORMALDEPTHMAP_HPP_
#define SIMULATION_NORMAL_DEPTH_MAP_SRC_NORMALDEPTHMAP_HPP_

#include <vector>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/ref_ptr>
#include <osg/TriangleFunctor>

namespace normal_depth_map {


  /**
   * @brief
   *
   */

struct TriangleStruct {
    osg::Vec3 _v_1;
    osg::Vec3 _v_2;
    osg::Vec3 _v_3;
    osg::Vec3 _normal;
    osg::Vec3 _centroid;

    TriangleStruct() : _v_1( osg::Vec3(0,0,0) ),
                       _v_2( osg::Vec3(0,0,0) ),
                       _v_3( osg::Vec3(0,0,0) ),
                       _normal( osg::Vec3(0,0,0) ),
                       _centroid( osg::Vec3(0,0,0) ){};

    TriangleStruct(osg::Vec3 v_1, osg::Vec3 v_2, osg::Vec3 v_3)
      : _v_1(v_1), _v_2(v_2), _v_3(v_3),
        _normal( osg::Vec3(0,0,0) ),
        _centroid( osg::Vec3(0,0,0) ){

        setTriangle(v_1, v_2, v_3);
    };

    void setTriangle(osg::Vec3 v_1, osg::Vec3 v_2, osg::Vec3 v_3){

        _v_1 = v_1;
        _v_2 = v_2;
        _v_3 = v_3;
        _centroid = (_v_1 + _v_2 + _v_3) / 3;

        osg::Vec3 v1_v2 = _v_2 - _v_1;
        osg::Vec3 v1_v3 = _v_3 - _v_1;
        _normal = v1_v2.operator ^(v1_v3);
        _normal.normalize();
    };

    bool operator < (const TriangleStruct& obj_1){

        if ( _centroid.x() < obj_1._centroid.x() )
            return true;
        else if ( _centroid.x() > obj_1._centroid.x() )
            return false;
        else if ( _centroid.y() < obj_1._centroid.y() )
            return true;
        else if ( _centroid.y() > obj_1._centroid.y() )
            return false;
        else if ( _centroid.z() < obj_1._centroid.z() )
            return true;

        return false;
    }
};


struct TrianglesCollection{

    std::vector<TriangleStruct>* triangles;
    osg::Matrix local_2_world;

    TrianglesCollection() {
        triangles = new std::vector<TriangleStruct>();
    };

    ~TrianglesCollection() {
        delete triangles;
    };

    inline void operator () (const osg::Vec3& v1,
                             const osg::Vec3& v2,
                             const osg::Vec3& v3,
                             bool treatVertexDataAsTemporary) {

        // transform vertice coordinates to world coordinates
        osg::Vec3 v1_w = v1 * local_2_world;
        osg::Vec3 v2_w = v2 * local_2_world;
        osg::Vec3 v3_w = v3 * local_2_world;
        triangles->push_back( TriangleStruct(v1_w, v2_w, v3_w) );
    };
};

/**
 * @brief
 *
 */
class TrianglesVisitor : public osg::NodeVisitor {
public:

    osg::TriangleFunctor<TrianglesCollection> triangles_data;

    TrianglesVisitor();
    void apply( osg::Geode& geode );
};



/**
 * @brief Gets the informations of normal and depth from a osg scene, between the objects and the camera.
 *
 * Using a shaders render process, it is build a map with two values:
 *  The dot product with normal vector of the object surface and the camera
 *    normal vector, called normal value;
 *  The distance between the objects and the camera, called depth value;
 *
 */
class NormalDepthMap {
public:
    /**
     * @brief Build a map informations from the normal surface and depth from objects to the camera.
     *
     * This class takes a osg node scene and apply the shaders
     *  to get the depth and normal surface information between the
     *  camera and the objects in the scene.
     *
     *  This class apply the shaders to get the normal and depth information, to build the map in osg::Node.
     *  BLUE CHANNEL, presents the normal values from the objects to the center camera, where:
     *      1 is the max value, and represents the normal vector of the object surface and the normal vector of camera are in the same directions, || ;
     *      0 is the minimum value, the normal vector of the object surface and the normal vector of camera are in the perpendicular directions, |_ ;
     *  GREEN CHANNEL presents the linear depth values relative from camera center in 8bits precision, where:
     *      0 is the minimum value, and represents the object is near from the camera;
     *      1 is the max value, and represents the object is far from the camera, and it is limited by max range;
     *  DEPTH BUFFER presents the linear depth values relative from camera center in 32bits precision, where:
     *      0 is the minimum value, and represents the object is near from the camera;
     *      1 is the max value, and represents the object is far from the camera, and it is limited by max range;
     *
     *  @param maxRange: It is a float value which limits the depth calculation process. Default maxRange = 50.0
     */
    NormalDepthMap();
    NormalDepthMap(float maxRange);
    NormalDepthMap(float maxRange, float attenuationCoeff);

    /**
     * @brief Add the models in the normal depth map node
     *  @param node: osg node to add a main scene
     */
    void addNodeChild(osg::ref_ptr<osg::Node> node);

    /**
     * @brief Get the node with the normal and depth map
     *  @param node: It is a node with the models in the target scene
     */
    const osg::ref_ptr<osg::Group> getNormalDepthMapNode() const {
        return _normalDepthMapNode;
    };

    void setMaxRange(float maxRange);
    float getMaxRange();

    void setAttenuationCoefficient(float coefficient);
    float getAttenuationCoefficient();

    void setDrawNormal(bool drawNormal);
    bool isDrawNormal();

    void setDrawDepth(bool drawDepth);
    bool isDrawDepth();


private:
    osg::ref_ptr<osg::Group> _normalDepthMapNode; //main shader node
    TrianglesVisitor _visitor;

    osg::ref_ptr<osg::Group> createTheNormalDepthMapShaderNode(
                              float maxRange = 50.0,
                              float attenuationCoefficient = 0,
                              bool drawDepth = true,
                              bool drawNormal = true);

    void convertVecticesToTexture(osg::ref_ptr<osg::Node> node);
};
}

#endif /* SIMULATION_NORMAL_DEPTH_MAP_SRC_NORMALDEPTHMAP_HPP_ */
