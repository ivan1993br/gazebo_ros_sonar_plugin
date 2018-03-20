// C++ includes
#include <iostream>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

// OSG includes
#include <osg/Geode>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgDB/ReadFile>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define BOOST_TEST_MODULE "Tools_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Tools)
BOOST_AUTO_TEST_CASE(setOSGImagePixel_TestCase) {

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(500, 500, 3, GL_RGB, GL_FLOAT);

    for (unsigned int x = 100; x < 200 ; x++) {
        for (unsigned int y = 10; y < 20 ; y++) {
            setOSGImagePixel(image, x, y, 0, 1.0f);
            setOSGImagePixel(image, x, y, 1, 1.0f);
            setOSGImagePixel(image, x, y, 2, 1.0f);
        }
    }

    for (unsigned int x = 300; x < 500 ; x++) {
        for (unsigned int y = 10; y < 100 ; y++) {
            setOSGImagePixel(image, x, y, 0, 1.0f);
            setOSGImagePixel(image, x, y, 1, 1.0f);
        }
    }

    for (unsigned int x = 0; x < 500 ; x++) {
        for (unsigned int y = 120; y < 200 ; y++) {
            setOSGImagePixel(image, x, y, 1, 1.0f);
        }
    }

    for (unsigned int x = 10; x < 50 ; x++) {
        for (unsigned int y = 210; y < 300 ; y++) {
            setOSGImagePixel(image, x, y, 2, 1.0f);
        }
    }

    cv::Mat cv_image = convertOSG2CV(image);
  	cv::imshow("test", cv_image);
  	cv::waitKey();
}

BOOST_AUTO_TEST_SUITE_END();
