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
#include <osgDB/ReadFile>

#include <iostream>
#include <algorithm>

namespace normal_depth_map {

#define FIRST_PASS_VERT "normal_depth_map/shaders/firstPass.vert"
#define FIRST_PASS_FRAG "normal_depth_map/shaders/firstPass.frag"
#define SECND_PASS_VERT "normal_depth_map/shaders/secondPass.vert"
#define SECND_PASS_FRAG "normal_depth_map/shaders/secondPass.frag"

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

osg::ref_ptr<osg::Camera> NormalDepthMap::createRTTCamera(osg::Camera::BufferComponent buffer,
                             osg::ref_ptr<osg::Texture2D> tex) {
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(osg::Vec4());
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    camera->attach(buffer, tex);
    return camera.release();
}

osg::ref_ptr<osg::Texture2D> createFloatTexture(int width, int height) {
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize(width, height);
    tex2D->setInternalFormat(GL_RGBA16F_ARB);
    tex2D->setSourceFormat(GL_RGBA);
    tex2D->setSourceType(GL_FLOAT);
    return tex2D.release();
}

osg::ref_ptr<osg::StateSet> setShaderProgram(osg::ref_ptr<osg::Camera> pass,
                                             const std::string& vert,
                                             const std::string& frag)
{
    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(osgDB::readRefShaderFile(vert));
    program->addShader(osgDB::readRefShaderFile(frag));
    osg::ref_ptr<osg::StateSet> ss = pass->getOrCreateStateSet();
    ss->setAttributeAndModes(
        program.get(),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    return ss;
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

    osg::ref_ptr<osg::Shader> firstPassVert = osg::Shader::readShaderFile(
                                                osg::Shader::VERTEX,
                                                osgDB::findDataFile(
                                                    FIRST_PASS_VERT));

    osg::ref_ptr<osg::Shader> firstPassFrag = osg::Shader::readShaderFile(
                                                osg::Shader::FRAGMENT,
                                                osgDB::findDataFile(
                                                    FIRST_PASS_FRAG));
    program->addShader(firstPassVert);
    program->addShader(firstPassFrag);

    osg::ref_ptr<osg::StateSet> ss = localRoot->getOrCreateStateSet();
    ss->setAttribute(program);

    ss->addUniform(new osg::Uniform("farPlane", maxRange));
    ss->addUniform(new osg::Uniform("attenuationCoeff",
                                    attenuationCoefficient));
    ss->addUniform(new osg::Uniform("drawNormal", drawNormal));
    ss->addUniform(new osg::Uniform("drawDepth", drawDepth));

    return localRoot;
}

} // namespace normal_depth_map
