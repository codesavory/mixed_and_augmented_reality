#include "cameraHelper.h"
#include <opencv2/core/types_c.h>

using namespace std;
using namespace cv;

int debug = 1;

CameraHelper::CameraHelper(){
  // NOTE - distortion_coeffs is not initialized because it may be variable size
  //        depending on the camera calibration
  intrinsic_ = Mat_<double>(3, 3);
  fovx_ = 0;
  fovy_ = 0;
  img_width_ = 0;
  img_height_ = 0;
  focal_length_ = 0;
  board_w_ = 0;
  board_h_ = 0;
  fNear_ = 0.5;
  fFar_ = 1000;
  initialized_ = false;
  aspect_ratio_ = 1.0;
  pp_ = Point2d(); 
}

CameraHelper::CameraHelper(char* filename, int board_w, int board_h, int img_width, int img_height){
  initialized_ = false;
  /*
    horizontal field of view,
    vertical field of view,
    focal length (assuming 1.0f for both apertureWidth and apertureHeight),
    principal point,
    aspect ratio parameters fx and fy,
    distortion parameters k1, k2, p1, and p2
  */

  board_w_ = board_w;
  board_h_ = board_h;
  img_width_ = img_width;
  img_height_ = img_height;

  fNear_ = 0.5;
  fFar_ = 1000;

  double cx, cy, fx, fy, k1, k2, p1, p2;
  ifstream infile (filename);
  if (infile.is_open()){
    infile >> fovx_;
    infile >> fovy_;
    infile >> focal_length_;
    infile >> cx;
    infile >> cy;
    infile >> fx;
    infile >> fy;
    infile >> k1;
    infile >> k2;
    infile >> p1;
    infile >> p2;
    infile.close();
  }

  intrinsic_ = (Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
  distortion_coeffs_ = (Mat_<double>(1, 4) << k1, k2, p1, p2);
  initialized_ = true;
}

void CameraHelper::calibrateFromImages(const vector<Mat>& image, const int board_w, const int board_h){

  //set the img_width and img_height private params
  img_width_ = image[0].size().width;
  img_height_ = image[0].size().height;

  board_w_ = board_w;
  board_h_ = board_h;

  Size board_size = Size(board_w, board_h);

  vector<Point2f> corners; //locations
  vector< vector<Point3f> > object_points; //3D points in world-space
  vector< vector<Point2f> > image_points; //2D points in image-space
  vector<Mat> rvecs;
  vector<Mat> tvecs;


  //for each image
  for(int i = 0; i < image.size(); i++){
    //Find Chessboard Corners
    bool found = findChessboardCorners(image[i], board_size, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FILTER_QUADS);

    //create 3D world-space points for chessboard corners
    vector<Point3f> obj;
    for(int j = 0; j < board_w*board_h; j++)
      obj.push_back(Point3f(j/board_w, j%board_w, 0.0f));
      //obj.push_back(Point3f(j%board_w, j/board_w, 0.0f));
      
    // (0,0),(0,1),(0,2)...(0,9),(1,0)...(1,9)(2,0)... ...(6,9)
    // opengl_cv has:
    // (0,0),(1,0),(2,0)...(9,0),(0,1)...(9,1)(0,2)... ...(9,6)


    image_points.push_back(corners);
    object_points.push_back(obj);

  }

  //set aspect ratios to 1
  intrinsic_.ptr<double>(0)[0] = 1;
  intrinsic_.ptr<double>(1)[1] = 1;

  distortion_coeffs_ = Mat::zeros(4, 1, CV_64F);  
  //calibrate
  calibrateCamera(object_points, image_points, image[0].size(), intrinsic_, distortion_coeffs_, rvecs, tvecs, cv::CALIB_FIX_K3);

  calibrationMatrixValues(intrinsic_, image[0].size(), 0.01, 0.01*img_height_/img_width_, fovx_, fovy_, focal_length_, pp_, aspect_ratio_);

  initialized_ = true;
  outputCameraParameters("params2.txt");
  printCameraParameters();
}

void CameraHelper::getGLViewportArray(int v_array[4]){
  if(!initialized_)
    return;
  v_array[0] = (int)(intrinsic_.at<double>(0,2) - img_width_/2.0);
  v_array[1] = (int)(img_height_/2.0 - intrinsic_.at<double>(1,2));
  v_array[2] = img_width_;
  v_array[3] = img_height_;

}

void CameraHelper::getGluPerspectiveArray(double glu_array[4]){
  if(!initialized_){
    //throw error
    return;
  }
  glu_array[0] = fovy_;
  glu_array[1] = (double)img_width_/img_height_;
  glu_array[2] = fNear_;
  glu_array[3] = fFar_;
}

double CameraHelper::getExtrinsicParams(Mat& image, Mat& rvec, Mat& tvec){
  if(!initialized_)
    return -1.0;

  vector<Point3f> obj_points;
  for(int j = 0; j < board_w_*board_h_; j++)
    obj_points.push_back(Point3f(j/board_w_, j%board_w_, 0.0f));
    //obj_points.push_back(Point3f(j%board_w_, j/board_w_, 0.0f));

  vector<Point2f> corners;
  bool found = findChessboardCorners(image, Size(board_w_, board_h_), corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FILTER_QUADS);
  if(found){
    Mat gray;
    cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
  }
  else{ // if corners are not found
    //throw error
    cout << "error: could not find corners" << endl;
    return -1.0;
  }

  // solve for rvec and tvec
  solvePnP(obj_points, corners, intrinsic_, distortion_coeffs_, rvec, tvec);

  // Draw points projected with OpenCV. Useful for verifying intrinsic/extrinsic parameters 
  // and debugging the OpenGL projection matrix.
  if (debug) {
    vector<cv::Point2f> projected;
    cv::projectPoints(obj_points, rvec, tvec, intrinsic_, distortion_coeffs_, projected);
    for (int i = 0; i < projected.size(); i++) {
      cv::Point2f pt = projected.at(i);
      cv::circle(image, pt, 5, cv::Scalar(0, 0, 255));
    }
  }

  // calculate reprojection error
  vector<Point2f> reproj_points;
  projectPoints(obj_points, rvec, tvec, intrinsic_, distortion_coeffs_, reproj_points);
  // calculate SSD
  double ssd = 0;
  for(int i = 0; i < obj_points.size(); i++){
    Point2d temp = corners[i] - reproj_points[i];
    ssd += pow(temp.x, 2) + pow(temp.y, 2);
  }
  return ssd;
}


int CameraHelper::getIntrinsicParams(double& fx, double& fy, double& fovx, double& fovy, double& ppx, double& ppy){
  if(!initialized_)
    return 0;
  
  fx = intrinsic_.at<double>(0,0);
  fy = intrinsic_.at<double>(1,1);
  fovx = fovx_; 
  fovy = fovy_;
  ppx = intrinsic_.at<double>(0,2);
  ppy = intrinsic_.at<double>(1,2);
  return 1;
}

void CameraHelper::undistortImage(Mat& image, Mat& corrected_image){
  if(!initialized_)
    return;
  cout << distortion_coeffs_ << endl;
  undistort(image, corrected_image, intrinsic_, distortion_coeffs_);
}

void CameraHelper::setFNearFFar(const double fNear, const double fFar){
  fNear_ = fNear;
  fFar_ = fFar;
}

void CameraHelper::printCameraParameters(){
  if(!initialized_)
    return;
  cout << "width, height = " << img_width_ << ", " << img_height_ << endl;
  cout << "fx, fy = " << intrinsic_.at<double>(0,0) << ", " << intrinsic_.at<double>(1,1) << endl;
  cout << "cx, cy = " << intrinsic_.at<double>(0,2) << ", " << intrinsic_.at<double>(1,2) << endl;
  cout << "fovx, fovy = " << fovx_ << ", " << fovy_ << endl;
  cout << "focal length = " << focal_length_ << endl;
}

void CameraHelper::outputCameraParameters(string filename){
  if(!initialized_)
    return;
  ofstream outfile (filename.c_str());
  if (outfile.is_open()){
    outfile << fovx_ << endl;
    outfile << fovy_ << endl;
    outfile << focal_length_ << endl;
    outfile << intrinsic_.at<double>(0,2) << endl;
    outfile << intrinsic_.at<double>(1,2) << endl;
    outfile << intrinsic_.at<double>(0,0) << endl;
    outfile << intrinsic_.at<double>(1,1) << endl;
    outfile << distortion_coeffs_.at<double>(0,0) << endl;
    outfile << distortion_coeffs_.at<double>(0,1) << endl;
    outfile << distortion_coeffs_.at<double>(0,2) << endl;
    outfile << distortion_coeffs_.at<double>(0,3) << endl;
    // TESTING:
    outfile << "aspect ratio: " << aspect_ratio_ << endl;
    outfile.close();
  }
}
