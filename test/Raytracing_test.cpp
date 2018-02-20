// C++ includes
#include <iostream>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Attenuation_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(RayTracing)

BOOST_AUTO_TEST_CASE(attenuationDemo_testCase) {
    // sonar parameters
    float maxRange = 50;            // 50 meters
    float fovX = M_PI / 6;    // 30 degrees
    float fovY = M_PI / 6;    // 30 degrees

     // define the different camera point of views
    std::vector<osg::Vec3d> eyes, centers, ups;
    viewPointsFromDemoScene(&eyes, &centers, &ups);

    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> root = new osg::Group();
    makeDemoScene(root);

    // display the same scene with and without underwater acoustic attenuation
    for (uint i = 0; i < eyes.size(); ++i) {
        cv::Mat rawShader = computeNormalDepthMap(root, maxRange, fovX, fovY, 0, eyes[i], centers[i], ups[i]);
        cv::Mat rawSonar  = drawSonarImage(rawShader, maxRange, fovX * 0.5);

        // output
        cv::imshow("raw shader", rawShader);
        cv::imshow("raw sonar ", rawSonar);
        cv::waitKey();
    }
}

BOOST_AUTO_TEST_SUITE_END();
