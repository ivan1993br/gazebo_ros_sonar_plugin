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

    // pass the triangles data to GLSL as uniform
    _normalDepthMapNode->accept(_visitor);
    std::sort( (*_visitor.triangles_data.triangles).begin(),
               (*_visitor.triangles_data.triangles).end());

    osg::ref_ptr<osg::StateSet> ss = _normalDepthMapNode->getOrCreateStateSet();
    osg::ref_ptr<osg::Texture2D> trianglesTexture;
    convertTrianglesToTextures(_visitor.triangles_data.triangles, trianglesTexture);
    ss->addUniform(new osg::Uniform(osg::Uniform::SAMPLER_2D, "trianglesTex"));
    ss->setTextureAttributeAndModes(0, trianglesTexture, osg::StateAttribute::ON);
}

osg::ref_ptr<osg::Group> NormalDepthMap::createTheNormalDepthMapShaderNode(
                                                float maxRange,
                                                float attenuationCoefficient,
                                                bool drawDepth,
                                                bool drawNormal) {

    osg::ref_ptr<osg::Group> localRoot = new osg::Group();
    osg::ref_ptr<osg::Program> program(new osg::Program());

    osg::ref_ptr<osg::Shader> pass1vert = osg::Shader::readShaderFile(
                                                osg::Shader::VERTEX,
                                                osgDB::findDataFile(PASS1_VERT_PATH));

    osg::ref_ptr<osg::Shader> pass1frag = osg::Shader::readShaderFile(
                                                osg::Shader::FRAGMENT,
                                                osgDB::findDataFile(PASS1_FRAG_PATH));
    program->addShader(pass1vert);
    program->addShader(pass1frag);

    osg::ref_ptr<osg::StateSet> ss = localRoot->getOrCreateStateSet();
    ss->setAttribute(program);

    ss->addUniform(new osg::Uniform("farPlane", maxRange));
    ss->addUniform(new osg::Uniform("attenuationCoeff", attenuationCoefficient));
    ss->addUniform(new osg::Uniform("drawNormal", drawNormal));
    ss->addUniform(new osg::Uniform("drawDepth", drawDepth));

    return localRoot;
}

} // namespace normal_depth_map
