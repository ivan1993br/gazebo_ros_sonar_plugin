#include "NormalDepthMap.hpp"

// C++ includes
#include <iostream>
#include <algorithm>

// OSG includes
#include <osg/Program>
#include <osg/ref_ptr>
#include <osg/Shader>
#include <osg/StateSet>
#include <osg/Uniform>
#include <osgDB/FileUtils>
#include <osg/ShapeDrawable>

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

    // sort the scene's triangles in ascending order (for each model)
    std::vector<Triangle> triangles = _visitor.getTriangles();
    std::vector<uint> trianglesRef = _visitor.getTrianglesRef();

    // for (size_t idx = 0; idx < trianglesRef.size() - 1; idx++)
        // std::sort(triangles.begin() + trianglesRef[idx], triangles.begin() + trianglesRef[idx + 1]);
    // std::sort(triangles.begin(), triangles.end());

    // convert triangles (data + reference) to osg texture
    osg::ref_ptr<osg::Texture2D> trianglesTexture;
    triangles2texture(triangles, trianglesRef, trianglesTexture);

    // pass the triangles (data + reference) to GLSL as uniform
    osg::ref_ptr<osg::StateSet> pass1state = _normalDepthMapNode->getChild(0)->getOrCreateStateSet();

    pass1state->addUniform(new osg::Uniform(osg::Uniform::SAMPLER_2D, "trianglesTex"));
    pass1state->setTextureAttributeAndModes(0, trianglesTexture, osg::StateAttribute::ON);

    // pass1state->addUniform(new osg::Uniform(osg::Uniform::FLOAT_VEC3, "trianglesTexSize"));
    // pass1state->getUniform("trianglesTexSize")->set(osg::Vec3(  trianglesRef.size() * 1.0,
    //                                                             trianglesTexture->getTextureWidth() * 1.0,
    //                                                             trianglesTexture->getTextureHeight() * 1.0));

    pass1state->addUniform(new osg::Uniform(osg::Uniform::FLOAT_VEC3, "trianglesTexSize"));
    pass1state->getUniform("trianglesTexSize")->set(osg::Vec3(  triangles.size() * 1.0,
                                                                trianglesTexture->getTextureWidth() * 1.0,
                                                                trianglesTexture->getTextureHeight() * 1.0));

    std::cout << "Dimensions x: " << triangles.size() << std::endl;
    std::cout << "Dimensions y: " << trianglesTexture->getTextureWidth() << std::endl;
    std::cout << "Dimensions z: " << trianglesTexture->getTextureHeight() << std::endl;

    // // pass the triangles reference to GLSL as uniform
    // // // int dataArray[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    // // // size_t dataArraySize = sizeof(dataArray) / sizeof(dataArray[0]);
    // // // std::vector<float> trianglesRefFloat(dataArray, dataArray + dataArraySize);

    // std::cout << "Number of triangles: " << triangles.size() << std::endl;
    // // for(size_t i = 0; i < trianglesRef.size(); i++)
    //     // std::cout << "TriangleRef: " << trianglesRef[i] << std::endl;
    std::vector<float> trianglesRefFloat(trianglesRef.begin(), trianglesRef.end());

    for(size_t i = 0; i < trianglesRefFloat.size(); i++)
        std::cout << "TrianglesRef: " << trianglesRefFloat[i] << std::endl;

    // osg::ref_ptr<osg::Image> image = new osg::Image();
    // image->allocateImage(trianglesRefFloat.size(), 1, 1, GL_RED, GL_FLOAT);
    // image->setInternalTextureFormat(GL_R32F);

    // for (size_t idx = 0; idx < trianglesRefFloat.size(); idx++)
    //     setOSGImagePixel(image, 0, idx, 0, trianglesRefFloat[idx]);

    // osg::ref_ptr<osg::Texture2D> trianglesRefTexture = new osg::Texture2D;
    // trianglesRefTexture->setTextureSize(image->s(), image->t());
    // trianglesRefTexture->setResizeNonPowerOfTwoHint(false);
    // trianglesRefTexture->setUnRefImageDataAfterApply(true);
    // trianglesRefTexture->setImage(image);


    // pass1state->addUniform(new osg::Uniform(osg::Uniform::SAMPLER_2D, "trianglesRefTex"));
    // pass1state->setTextureAttributeAndModes(1, trianglesRefTexture, osg::StateAttribute::ON);



    // pass1state->addUniform(new osg::Uniform(osg::Uniform::FLOAT_VEC2, "trianglesRefTexSize"));
    // pass1state->getUniform("trianglesRefTexSize")->set(osg::Vec2(   trianglesRefTexture->getTextureWidth() * 1.0,
    //                                                                 trianglesRefTexture->getTextureHeight() * 1.0));
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
    pass1state->addUniform(new osg::Uniform(osg::Uniform::BOOL, "drawDistance"));
    pass1state->addUniform(new osg::Uniform(osg::Uniform::BOOL, "drawNormal"));
    pass1state->getUniform("farPlane")->set(maxRange);
    pass1state->getUniform("attenuationCoeff")->set(attenuationCoefficient);
    pass1state->getUniform("drawDistance")->set(drawDepth);
    pass1state->getUniform("drawNormal")->set(drawNormal);

    // 2nd pass: secondary reflections by ray-triangle intersection
    osg::ref_ptr<osg::Group> pass2root = new osg::Group();
    osg::ref_ptr<osg::Program> pass2prog = new osg::Program();
    osg::ref_ptr<osg::StateSet> pass2state = pass2root->getOrCreateStateSet();
    pass2prog->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(PASS2_VERT_PATH)));
    pass2prog->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(PASS2_FRAG_PATH)));
    pass2state->setAttributeAndModes( pass2prog, osg::StateAttribute::ON );

    // set the main root
    localRoot->addChild(pass1root);
    localRoot->addChild(pass2root);

    return localRoot;
}

} // namespace normal_depth_map
