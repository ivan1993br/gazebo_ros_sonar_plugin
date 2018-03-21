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

    cv::Point ground_truth_points[] = {
                                  cv::Point(120,15),
                                  cv::Point(199,11),
                                  cv::Point(350,90),
                                  cv::Point(20,180),
                                  cv::Point(300,199),
                                  cv::Point(40,290),
                                  cv::Point(300,300),
                                  cv::Point(350,400),
                                  cv::Point(400,100) };

      cv::Point3f ground_truth_value[] = {
                                  cv::Point3f(1, 1, 1),
                                  cv::Point3f(1, 1, 1),
                                  cv::Point3f(0, 1, 1),
                                  cv::Point3f(0, 1, 0),
                                  cv::Point3f(0, 1, 0),
                                  cv::Point3f(1, 0, 0),
                                  cv::Point3f(0, 0, 1),
                                  cv::Point3f(1, 1, 0),
                                  cv::Point3f(0, 0, 0)};

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(500, 500, 3, GL_RGB, GL_FLOAT);

    for (unsigned int x = 100; x < 200 ; x++)
        for (unsigned int y = 10; y < 20 ; y++) {
            setOSGImagePixel(image, x, y, 0, 1.0f);
            setOSGImagePixel(image, x, y, 1, 1.0f);
            setOSGImagePixel(image, x, y, 2, 1.0f);
        }

    for (unsigned int x = 300; x < 400 ; x++)
        for (unsigned int y = 10; y < 100 ; y++) {
            setOSGImagePixel(image, x, y, 0, 1.0f);
            setOSGImagePixel(image, x, y, 1, 1.0f);
        }

    for (unsigned int x = 5; x < 490 ; x++)
        for (unsigned int y = 120; y < 200 ; y++) {
            setOSGImagePixel(image, x, y, 1, 1.0f);
        }

    for (unsigned int x = 10; x < 50 ; x++)
        for (unsigned int y = 210; y < 300 ; y++) {
            setOSGImagePixel(image, x, y, 2, 1.0f);
        }

    for (unsigned int x = 200; x < 490 ; x++)
        for (unsigned int y = 250; y < 350 ; y++) {
            setOSGImagePixel(image, x, y, 0, 1.0f);
        }

    for (unsigned int x = 50; x < 400 ; x++)
        for (unsigned int y = 380; y < 480 ; y++) {
            setOSGImagePixel(image, x, y, 1, 1.0f);
            setOSGImagePixel(image, x, y, 2, 1.0f);
        }

    cv::Mat cv_image = convertOSG2CV(image);
    for (unsigned int i = 0; i < 9; i++) {
        cv::Point point = ground_truth_points[i];
        cv::Point3f value( cv_image.at<cv::Vec3f>(point.x,point.y)[0],
                           cv_image.at<cv::Vec3f>(point.x,point.y)[1],
                           cv_image.at<cv::Vec3f>(point.x,point.y)[2]);
        BOOST_CHECK_EQUAL(value, ground_truth_value[i]);

        //debug
        // std::cout << "cv::Point3f("
        //           << value.x <<", "
        //           << value.y <<", "
        //           << value.z
        //           << ")," << std::endl;
    }
    //debug
  	// cv::imshow("test", cv_image);
  	// cv::waitKey();
}

BOOST_AUTO_TEST_SUITE_END();
