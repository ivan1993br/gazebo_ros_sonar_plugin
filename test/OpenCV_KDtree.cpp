// C++ includes
#include <iostream>
#include <vector>

// Rock includes
// #include "TestHelper.hpp"

// OpenCV
#include <opencv2/core/core.hpp>

#define BOOST_TEST_MODULE "OpenCV_KDtree_test"
#include <boost/test/unit_test.hpp>


// code based on https://docs.opencv.org/ref/2.4/df/d23/classcv_1_1KDTree.html#a23e9decfb1caab6d0fe533956c99323e
BOOST_AUTO_TEST_CASE(initial_test_3D) {

  std::cout <<"\n3D Test" << std::endl;
  cv::Mat points = (cv::Mat_<float>(5,3) <<
    1, 6, 1,
    2, 5, 5,
    3, 9, 6,
    4, 3, 6,
    5, 4, 9);
  std::cout <<"Input Points\n "<< points << std::endl;

  cv::Mat query = (cv::Mat_<float>(1,3) <<
    4, 4, 6);

  cv::KDTree tree(points, true);
  const int K = 3, Emax = INT_MAX;
  cv::Mat idx, neighbors, dist;
  tree.findNearest(query, K, Emax, idx, neighbors, dist);

  for (int i = 0; i < K; i++) {
    std::cout << "IDX: " << idx.at<int>(i)
              << " | DIST: " << dist.at<float>(i)
              << " | NEIGHBOR: " << neighbors.at<cv::Vec3f>(i)
              << std::endl;
  }

  std::cout << "\nTree Points:\n " << tree.points << std::endl;

}

BOOST_AUTO_TEST_CASE(initial_test_2D) {

  std::cout <<"\n2D Test" << std::endl;
  cv::Mat points = (cv::Mat_<float>(5,3) <<
    4, 4, 0,
    5, 5, 0,
    5, 1, 0,
    4, 2, 0,
    3, 3, 0);
  std::cout <<"Input Points\n "<< points << std::endl;

  cv::Mat query = (cv::Mat_<float>(1,3) <<
    4, 1, 0);

  cv::KDTree tree(points, true);
  const int K = 3, Emax = INT_MAX;
  cv::Mat idx, neighbors, dist;
  tree.findNearest(query, K, Emax, idx, neighbors, dist);

  for (int i = 0; i < K; i++) {
    std::cout << "IDX: " << idx.at<int>(i)
              << " | DIST: " << dist.at<float>(i)
              << " | NEIGHBOR: " << neighbors.at<cv::Vec3f>(i)
              << std::endl;
  }

  std::cout << "\nTree Points:\n " << tree.points << std::endl;

}
