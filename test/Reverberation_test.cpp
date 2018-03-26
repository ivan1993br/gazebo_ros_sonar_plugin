// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TriangleFunctor>
#include <osgDB/FileUtils>

// C++ includes
#include <iostream>
#include <vector>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Reverberation_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Reverberation)

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

BOOST_AUTO_TEST_CASE(vector2osgImage_testCase1) {
    // original vector data
    std::vector<float> dataIn;
    for (size_t i = 0; i < 48; i++)
        dataIn.push_back(i * i * 0.01);

    // pass the data from vector to osg image
    osg::ref_ptr<osg::Image> image;
    vec2osgimg(dataIn, image);

    // compare them!
    std::vector<float> dataOut;
    osgimg2vec(image, dataOut);
    BOOST_CHECK(areEqualVectors(dataIn, dataOut) == true);
}

BOOST_AUTO_TEST_CASE(vector2osgImage_testCase2) {
    // original vector data
    std::vector<osg::Vec3> dataIn;
    for (size_t i = 0; i < 8; i++) {
        float value = i * i * 0.01;
        dataIn.push_back(osg::Vec3(value, value, value));
    }

    // pass the data from vector to osg image
    osg::ref_ptr<osg::Image> image;
    vec2osgimg(dataIn, image);

    // compare them!
    std::vector<osg::Vec3> dataOut;
    osgimg2vec(image, dataOut);
    BOOST_CHECK(areEqualVectors(dataIn, dataOut) == true);
}

BOOST_AUTO_TEST_CASE(reverberation_testCase) {
    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> scene = new osg::Group();
    makeDemoScene(scene);

    // sonar parameters
    float maxRange = 50;      // 50 meters
    float fovX = M_PI / 6;    // 30 degrees
    float fovY = M_PI / 6;    // 30 degrees

    // define the different camera point of views
    std::vector<osg::Vec3d> eyes, centers, ups;
    viewPointsFromDemoScene(&eyes, &centers, &ups);

    // compute and display the final shader and sonar images
    for (unsigned i = 0; i < eyes.size(); ++i) {
        cv::Mat rawShader = computeNormalDepthMap(scene, maxRange, fovX, fovY, 0, eyes[i], centers[i], ups[i]);
        cv::imshow("rawShader", rawShader);
        cv::waitKey();
    }
}

BOOST_AUTO_TEST_SUITE_END();
