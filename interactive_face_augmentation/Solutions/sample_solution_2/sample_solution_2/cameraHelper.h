#ifndef CAMERA_H
#define CAMERA_H

#include <opencv/cv.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <time.h>

using namespace std;
using namespace cv;

class CameraHelper{
 public:
  CameraHelper();
  //loads intrinsic parameters from file
  CameraHelper(string filename);
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

  void calcSolvePnP(vector<Point3f> &objectPoints, vector<Point2f> &imagePoints, Mat &rvec, Mat &tvec);

  void undistortImage(Mat& image, Mat& corrected_image);

  // input param - rvec and tvec calculated from getExtrinsicParams
  // output param - 16 member array in OpenGL style so that it can be used
  //          with glLoadMatrixd(array)
  // is static because it is a helper function that does not depend on class variables
  static void convertToGLModelviewArray(const Mat& rvec, const Mat& tvec, double gl_array[16]){
    // convert rotation vector to a matrix
    Mat rmat;
    Rodrigues(rvec, rmat);

    double transform[] = { rmat.at<double>(0, 0), rmat.at<double>(1, 0),
    			rmat.at<double>(2, 0), 0.0f, rmat.at<double>(0, 1), rmat.at<double>(
    					1, 1), rmat.at<double>(2, 1), 0.0f, rmat.at<double>(0, 2),
    			rmat.at<double>(1, 2), rmat.at<double>(2, 2), 0.0f, tvec.at<double>(
    					0), tvec.at<double>(1), tvec.at<double>(2), 1.0f };

    for (int i = 0; i < 16; ++i) {
		gl_array[i] = transform[i];
	}
  }

  void setFNearFFar(const double fNear, const double FFar);

  void printCameraParameters();

  double getFy();

  void outputCameraParameters(string filename);

 private:
  bool initialized_;
  Mat intrinsic_;
  Mat distortion_coeffs_;
  Size img_size_;
  double fovx_, fovy_, focal_length_;
  double fNear_, fFar_;
  double aspect_ratio;
};


#endif //CAMERA_H

