// OSG includes
#include <osgViewer/Viewer>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TriangleFunctor>
#include <osgDB/FileUtils>
#include <osgDB/WriteFile>

// C++ includes
#include <iostream>

// Rock includes
#include <normal_depth_map/Tools.hpp>
#include "TestHelper.hpp"

#define BOOST_TEST_MODULE "Reverberation2_test"
#include <boost/test/unit_test.hpp>

using namespace normal_depth_map;
using namespace test_helper;

BOOST_AUTO_TEST_SUITE(Reverberation2)

osg::ref_ptr<osg::Image> matToOsgImage(cv::Mat& mat) {
	cv::Mat rgb;
	cv::cvtColor(mat, rgb, CV_BGR2RGB);
	cv::flip(rgb, rgb, 0);

	osg::ref_ptr<osg::Image> image = new osg::Image;

	uchar *data = new uchar[rgb.total() * rgb.elemSize()];
	memcpy(data, rgb.data, rgb.total() * rgb.elemSize());
	image->setImage(mat.cols, mat.rows, 1, GL_RGBA32F_ARB, GL_RGB, GL_FLOAT, data, osg::Image::NO_DELETE);
    return image;
}

BOOST_AUTO_TEST_CASE(reverberation2_testCase) {
    // create a simple scene with multiple objects
    osg::ref_ptr<osg::Group> scene = new osg::Group();
    makeDemoScene(scene);

    // create sample image on OpenCV
    // unsigned int texSize = 512;
    unsigned int width = 256, height = 128;

    cv::Mat src = cv::Mat::zeros(height, width, CV_32FC3);
    src(cv::Rect(          0,            0, width * 0.5 - 1, height * 0.5 - 1)).setTo(cv::Scalar(1,0,0));
    src(cv::Rect(width * 0.5,            0, width * 0.5 - 1, height * 0.5 - 1)).setTo(cv::Scalar(0,1,0));
    src(cv::Rect(          0, height * 0.5, width * 0.5 - 1, height * 0.5 - 1)).setTo(cv::Scalar(0,0,1));
    src(cv::Rect(width * 0.5, height * 0.5, width * 0.5 - 1, height * 0.5 - 1)).setTo(cv::Scalar(1,1,1));

    // src(cv::Rect(            0,             0, 255, 255)).setTo(cv::Scalar(1,0,0));
    // src(cv::Rect(texSize * 0.5,             0, 255, 255)).setTo(cv::Scalar(0,1,0));
    // src(cv::Rect(            0, texSize * 0.5, 255, 255)).setTo(cv::Scalar(0,0,1));
    // src(cv::Rect(texSize * 0.5, texSize * 0.5, 255, 255)).setTo(cv::Scalar(1,1,1));

    // convert image from opencv to osg
    osg::ref_ptr<osg::Image> image = matToOsgImage(src);

    // convert image to texture
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setTextureSize(image->s(), image->t());
    texture->setResizeNonPowerOfTwoHint(false);
    texture->setUnRefImageDataAfterApply(true);
    texture->setImage(image);

    // Pass the texture to GLSL as uniform
    osg::StateSet* ss = scene->getOrCreateStateSet();
    osg::Uniform* texUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "vertexMap");
    texUniform->set(0);
    ss->addUniform(texUniform);
    ss->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

    // uniform with texture size
    // osg::Vec2 vertexSize = osg::Vec2(texSize, texSize);
    // ss->addUniform(new osg::Uniform("vertexSize", vertexSize));

    // sonar parameters
    float maxRange = 50;      // 50 meters
    float fovX = M_PI / 6;    // 30 degrees
    float fovY = M_PI / 6;    // 30 degrees

    osg::Vec3 eye(0,0,0), center(0,0,-1), up(0,1,0);
    cv::Mat dst = computeNormalDepthMap(scene, maxRange, fovX, fovY, 0, eye, center, up, 512);

    // data information
    std::cout << "Src image size: " << image->s() << "," << image->t() << std::endl;
    std::cout << "Texture size: " << texture->getTextureWidth() << "," << texture->getTextureHeight() << std::endl;
    std::cout << "Dst image size: " << dst.cols << "," << dst.rows << std::endl;
    std::cout << "Equals? " << (areEquals(src,dst)? "Yes" : "No") << std::endl;

    // output
    cv::imshow("src", src);     cv::waitKey();
    cv::imshow("dst", dst);     cv::waitKey();

    // // save image to file
    // osg::ref_ptr<osg::Image> image2 = texture->getImage();
    // osgDB::writeImageFile(*image2, "test.jpg");
    // std::cout << "Image2 size: " << image2->s() << "," << image2->t() << std::endl;
}

BOOST_AUTO_TEST_SUITE_END();
