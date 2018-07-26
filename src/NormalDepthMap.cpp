/*
 * NormalDepthMap.cpp
 *
 *  Created on: Mar 27, 2015
 *      Author: tiagotrocoli
 */

#include "NormalDepthMap.hpp"

#include <osg/Program>
#include <osg/ref_ptr>
#include <osg/Shader>
#include <osg/StateSet>
#include <osg/Uniform>
#include <osgDB/FileUtils>
#include <osg/ShapeDrawable>

#include <iostream>
#include <algorithm>

namespace normal_depth_map {

#define PASS1_VERT_PATH "normal_depth_map/shaders/pass1.vert"
#define PASS1_FRAG_PATH "normal_depth_map/shaders/pass1.frag"
#define PASS2_VERT_PATH "normal_depth_map/shaders/pass2.vert"
#define PASS2_FRAG_PATH "normal_depth_map/shaders/pass2.frag"

NormalDepthMap::NormalDepthMap(float maxRange ) {
    _normalDepthMapNode = createTheNormalDepthMapShaderNode(maxRange);
}

NormalDepthMap::NormalDepthMap(float maxRange, float attenuationCoeff) {
    _normalDepthMapNode = createTheNormalDepthMapShaderNode(
                                                          maxRange,
                                                          attenuationCoeff);
}

NormalDepthMap::NormalDepthMap() {
    _normalDepthMapNode = createTheNormalDepthMapShaderNode();
}

void NormalDepthMap::setMaxRange(float maxRange) {
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("farPlane")
                       ->set(maxRange);
}

float NormalDepthMap::getMaxRange() {
    float maxRange = 0;
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("farPlane")
                       ->get(maxRange);
    return maxRange;
}

void NormalDepthMap::setAttenuationCoefficient(float coefficient) {
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("attenuationCoeff")
                       ->set(coefficient);
}

float NormalDepthMap::getAttenuationCoefficient() {
    float coefficient = 0;
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("attenuationCoeff")
                       ->get(coefficient);
    return coefficient;
}

void NormalDepthMap::setDrawNormal(bool drawNormal) {
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("drawNormal")
                       ->set(drawNormal);
}

bool NormalDepthMap::isDrawNormal() {
    bool drawNormal;
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("drawNormal")
                       ->get(drawNormal);
    return drawNormal;
}

void NormalDepthMap::setDrawDepth(bool drawDepth) {
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("drawDepth")
                       ->set(drawDepth);
}

bool NormalDepthMap::isDrawDepth() {
    bool drawDepth;
    _normalDepthMapNode->getOrCreateStateSet()
                       ->getUniform("drawDepth")
                       ->get(drawDepth);
    return drawDepth;
}

void NormalDepthMap::addNodeChild(osg::ref_ptr<osg::Node> node) {
    _normalDepthMapNode->addChild(node);

    // collect all triangles of scene
    _normalDepthMapNode->accept(_visitor);

    // store all triangles as a balanced kd-tree
    TriangleStruct* tree = makeTree(&_visitor.triangles_data.triangles[0], _visitor.triangles_data.triangles.size(), 0, 3);

    // std::cout << "Height: " << height(tree) << std::endl;

    // organize the triangles (nodes) of balanced kd-tree in vertical order
    std::vector<TriangleStruct> triangles;
    levelOrder(tree, triangles);

    // std::cout << "Teste" << std::endl;
    // std::cout << "A: " << triangles[0].data[0][0] << "," << triangles[0].data[0][1] << "," << triangles[0].data[0][2] << std::endl;
    // std::cout << "B: " << triangles[0].data[1][0] << "," << triangles[0].data[1][1] << "," << triangles[0].data[1][2] << std::endl;
    // std::cout << "C: " << triangles[0].data[2][0] << "," << triangles[0].data[2][1] << "," << triangles[0].data[2][2] << std::endl;

    // convert triangles to texture
    osg::ref_ptr<osg::Texture2D> trianglesTexture;
    convertTrianglesToTextures(triangles, trianglesTexture);

    // pass the triangles data to GLSL as uniform
    osg::ref_ptr<osg::StateSet> pass2state = _normalDepthMapNode->getChild(1)->getOrCreateStateSet();
    pass2state->addUniform(new osg::Uniform(osg::Uniform::SAMPLER_2D, "trianglesTex"));
    pass2state->setTextureAttributeAndModes(0, trianglesTexture, osg::StateAttribute::ON);

    pass2state->addUniform(new osg::Uniform(osg::Uniform::FLOAT_VEC2, "trianglesTexSize"));
    pass2state->getUniform("trianglesTexSize")->set(osg::Vec2(  trianglesTexture->getTextureWidth() * 1.0,
                                                                trianglesTexture->getTextureHeight() * 1.0));

    // std::cout << "Size: " << trianglesTexture->getTextureWidth() << "," << trianglesTexture->getTextureHeight() << std::endl;
}

osg::ref_ptr<osg::Group> NormalDepthMap::createTheNormalDepthMapShaderNode(
                                                float maxRange,
                                                float attenuationCoefficient,
                                                bool drawDepth,
                                                bool drawNormal) {

    osg::ref_ptr<osg::Group> localRoot = new osg::Group();

    // 1st pass: primary reflections by rasterization pipeline
    osg::ref_ptr<osg::Group> pass1root = new osg::Group();
    osg::ref_ptr<osg::Program> pass1prog = new osg::Program();
    osg::ref_ptr<osg::StateSet> pass1state = pass1root->getOrCreateStateSet();
    pass1prog->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(PASS1_VERT_PATH)));
    pass1prog->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(PASS1_FRAG_PATH)));
    pass1state->setAttributeAndModes( pass1prog, osg::StateAttribute::ON );

    // 1st pass: uniforms
    pass1state->addUniform(new osg::Uniform(osg::Uniform::FLOAT, "farPlane"));
    pass1state->addUniform(new osg::Uniform(osg::Uniform::FLOAT, "attenuationCoeff"));
    pass1state->addUniform(new osg::Uniform(osg::Uniform::BOOL, "drawDepth"));
    pass1state->addUniform(new osg::Uniform(osg::Uniform::BOOL, "drawNormal"));
    pass1state->getUniform("farPlane")->set(maxRange);
    pass1state->getUniform("attenuationCoeff")->set(attenuationCoefficient);
    pass1state->getUniform("drawDepth")->set(drawDepth);
    pass1state->getUniform("drawNormal")->set(drawNormal);

    // 2nd pass: secondary reflections by ray-triangle intersection
    osg::ref_ptr<osg::Group> pass2root = new osg::Group();
    osg::ref_ptr<osg::Program> pass2prog = new osg::Program();
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    pass2prog->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(PASS2_VERT_PATH)));
    pass2prog->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(PASS2_FRAG_PATH)));
    pass2state->setAttributeAndModes( pass2prog, osg::StateAttribute::ON );

    // // 2nd pass: uniforms
    // pass2state->addUniform(new osg::Uniform(osg::Uniform::FLOAT, "farPlane"));
    // pass2state->getUniform("farPlane")->set(maxRange);

    // set the main root
    localRoot->addChild(pass1root);
    localRoot->addChild(pass2root);

    return localRoot;
}

} // namespace normal_depth_map
