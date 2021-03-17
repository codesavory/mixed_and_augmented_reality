#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>

using namespace std;
using namespace cv;

class CameraHelper{
 public:
  CameraHelper();
  //loads intrinsic parameters from file
  CameraHelper(char* filename, int board_w, int board_h, int img_width, int img_height);
  ~CameraHelper();

  /*
    takes in a vector of images that will be used to derive intrinsic parameters
    assuming that the corners detected will be at (0, 0, 0), (0, 0, 1), etc.
  */
  void calibrateFromImages(const vector<Mat> &image, const int board_w, const int board_h);

  // output param - viewport array to be used in glViewport method
  // array is adjusted for principle point
  void getGLViewportArray(int v_array[4]);

  // returns an array used for gluPerspective method in OpenGL
  // sets values to fovy, (openGL) aspect ratio, fNear, fFar
  void getGluPerspectiveArray(double glu_array[4]);

  // input param - image to calculate the pose for
  // output param - rvec and tvec corresponding to extrinsic params
  // returns - reprojection error (SSD)
  double getExtrinsicParams(Mat& image, Mat& rvec, Mat& tvec);

  int getIntrinsicParams(double& fx, double& fy, double& fovx, double& fovy, double& ppx, double& ppy);

  void undistortImage(Mat& image, Mat& corrected_image);

  // input param - rvec and tvec calculated from getExtrinsicParams
  // output param - 16 member array in OpenGL style so that it can be used
  //          with glLoadMatrixd(array)
  // is static because it is a helper function that does not depend on class variables
  static void convertToGLModelviewArray(const Mat& rvec, const Mat& tvec, double gl_array[16]){
    // convert rotation vector to a matrix
    Mat rot;
    Rodrigues(rvec, rot);

    gl_array[0] = rot.at<double>(0,0);
    gl_array[1] = rot.at<double>(1,0);
    gl_array[2] = rot.at<double>(2,0);
    gl_array[3] = 0.0;
    gl_array[4] = rot.at<double>(0,1);
    gl_array[5] = rot.at<double>(1,1);
    gl_array[6] = rot.at<double>(2,1);
    gl_array[7] = 0.0;
    gl_array[8] = rot.at<double>(0,2);
    gl_array[9] = rot.at<double>(1,2);
    gl_array[10] = rot.at<double>(2,2);
    gl_array[11] = 0.0;
    gl_array[12] = tvec.at<double>(0);
    gl_array[13] = tvec.at<double>(1);
    gl_array[14] = tvec.at<double>(2);
    gl_array[15] = 1.0;

  }

  void setFNearFFar(const double fNear, const double FFar);

  void printCameraParameters();

  void outputCameraParameters(string filename);

 private:
  bool initialized_;
  Mat intrinsic_;
  Mat distortion_coeffs_;
  int img_width_, img_height_;
  int board_w_, board_h_;
  double fovx_, fovy_, focal_length_;
  double fNear_, fFar_;
  double aspect_ratio_;
  Point2d pp_; 
};


#endif //CAMERA_H

