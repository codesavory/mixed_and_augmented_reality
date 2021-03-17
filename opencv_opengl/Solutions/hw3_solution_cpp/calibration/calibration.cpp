#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/calib3d/calib3d.hpp"

#include <vector>
#include <stdio.h>
#include <math.h>

using namespace std;

static bool debug = false;

// Finds the chessboard corners in an image file.
bool getImagePoints(const char* imageName, int width, int height, vector<cv::Point2f>& corners) {
  cv::Mat image = cv::imread(imageName, 1);
  if (!image.data) {
    fprintf(stderr, "File '%s' contains no image data.\n", imageName);
    return false;
  }

  cv::Size patternSize(width, height);
  bool boardFound = cv::findChessboardCorners(image, patternSize, corners, 0);

  fprintf(stderr, "\tPattern %s\n", boardFound ? "found" : "not found");
  if (debug) {
    // Draw found points.
    cv::drawChessboardCorners(image, patternSize, corners, boardFound);
    cv::namedWindow(imageName, CV_WINDOW_AUTOSIZE);
    cv::imshow(imageName, image);
  }
  return boardFound;
}


// Creates a vector containing 3D coordinates of chessboard corners.
void createObjectPoints(int width, int height, vector<cv::Point3f>& points) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      points.push_back(cv::Point3f(x, y, 0));
    }
  }
}


// Returns the dimensions of an image file.
cv::Size getImageSize(const char* imageName) {
  cv::Mat image = cv::imread(imageName, 1);
  return cv::Size(image.cols, image.rows);
}


int main( int argc, char** argv ) {
  FILE* output = stdout;
  // Or, if you'd prefer a file directly.
  // FILE *output = fopen("params.txt", "w");

  if (argc < 2) {
    fprintf(stderr, "Usage: calibrate [--debug_output] image1.png ... imageN.png > params.txt\n");
    fprintf(stderr, "\t--debug_output: Display found and reprojected corner points.\n");
    return 1;
  }

  int firstImage = 1;
  if (!strcmp("--debug_output", argv[1])){
    firstImage = 2;
    debug = true;
  }

  // Dimensions of calibration target.
  int width = 10;
  int height = 7;
  int imageCount = argc - firstImage;

  // Get the object points.
  vector<vector<cv::Point3f> > objectPoints;
  for (int i = 0; i < imageCount; i++) {
    vector<cv::Point3f> points;
    createObjectPoints(width, height, points);
    objectPoints.push_back(points);
  }
  
  // Get the image points.
  vector<vector<cv::Point2f> > imagePoints;
  for (int i = 0; i < imageCount; i++) {
    char* imageName = argv[firstImage + i];
    fprintf(stderr, "Process file [%s]\n", imageName);    
    
    vector<cv::Point2f> points;
    bool found = getImagePoints(imageName, width, height, points);
    if (!found) {
      fprintf(stderr, "Pattern not found! Aborted.\n");
      return 1;
    }
    imagePoints.push_back(points);
  }

  // And calibrate!
  cv::Mat cameraMat;
  cv::Mat distCoeffs;
  vector<cv::Mat> rvecs;
  vector<cv::Mat> tvecs;
  // K3 fixed so that only the first 4 distortion coefficients are used (as per instructions).
  double error = cv::calibrateCamera(objectPoints, imagePoints, getImageSize(argv[firstImage]), 
				     cameraMat, distCoeffs, rvecs, tvecs, CV_CALIB_FIX_K3);
  
  // Calculate reprojection error.
  double totalSsd = 0;
  double totalPixel = 0;
  for (int i = 0; i < imageCount; i++) {
    vector<cv::Point2f> reprojectedPoints;
    projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMat, distCoeffs, reprojectedPoints);

    double ssd = 0;
    double pixel = 0;
    for (int j = 0; j < reprojectedPoints.size(); j++) {
      cv::Point2f a = reprojectedPoints[j];
      cv::Point2f b = imagePoints.at(i).at(j);
      if (debug) {
	fprintf(stderr, "Found:     %f,%f\n",b.x,b.y);
	fprintf(stderr, "Projected: %f,%f\n",a.x,a.y);
      }
      double distance2 = pow(a.x-b.x,2) + pow(a.y-b.y,2); 
      ssd += distance2;
      double distance = sqrt(distance2);
      pixel += distance;
    }

    fprintf(stderr, "SSD[%d]: %f\n", i, ssd);
    fprintf(stderr, "avg pixel[%d]: %f\n", i, pixel/(width * height));

    totalSsd += ssd;
    totalPixel += pixel;

    if (debug) {
      // Draw reprojected points.
      cv::Mat image = cv::imread(argv[firstImage + i], 1);
      for (int j = 0; j < reprojectedPoints.size(); j++) {
	cv::Point2f point = reprojectedPoints[j];
	cv::circle(image, point, 5, cv::Scalar(0, 0, 255));
      }
      char winName[100];
      sprintf(winName, "reprojected %d", i);
      cv::namedWindow(winName, CV_WINDOW_AUTOSIZE);
      cv::imshow(winName, image);
    }
    
  }

  fprintf(stderr, "\n\033[1mCalibration complete.\033[0m\nRMS error: %f\n", error);
  fprintf(stderr, "SSD error: %f\n", totalSsd);
  fprintf(stderr, "Average pixel error: %f\n", totalPixel / (width * height * imageCount));
  fprintf(stderr, "Writing parameters to STDOUT...\n\n");

  double fovx, fovy, focalLength, aspectRatio;
  cv::Point2d principalPt;
  cv::Size imgSize = getImageSize(argv[firstImage]);
  cv::calibrationMatrixValues(cameraMat, imgSize, 0, 0, fovx, fovy, 
			      focalLength, principalPt, aspectRatio);
  //0,0 should be 1,1 ? aperture init

  // Write output.
  fprintf(output, "%f\n", fovx); // h fov
  fprintf(output, "%f\n", fovy); // v fov
  fprintf(output, "%f\n", focalLength); // focal length
  fprintf(output, "%f\n", principalPt.x); // principal x
  fprintf(output, "%f\n", principalPt.y); // principal y
  fprintf(output, "%f\n", cameraMat.at<double>(0, 0)); // fx
  fprintf(output, "%f\n", cameraMat.at<double>(1, 1)); // fy
  fprintf(output, "%f\n", distCoeffs.at<double>(0, 0)); // k1
  fprintf(output, "%f\n", distCoeffs.at<double>(0, 1)); // k2
  fprintf(output, "%f\n", distCoeffs.at<double>(0, 2)); // p1
  fprintf(output, "%f\n", distCoeffs.at<double>(0, 3)); // p2

  if (debug) {
    fprintf(stderr, "Wait\n");
    cv::waitKey(0);
  }

  return 0;
}
