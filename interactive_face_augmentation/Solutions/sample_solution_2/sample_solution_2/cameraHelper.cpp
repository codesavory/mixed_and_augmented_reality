#include "cameraHelper.h"


using namespace std;
using namespace cv;

CameraHelper::CameraHelper(){
  // NOTE - distortion_coeffs is not initialized because it may be variable size
  //        depending on the camera calibration
  intrinsic_ = Mat_<double>(3, 3);
  fovx_ = 0;
  fovy_ = 0;
  img_size_.width = 0;
  img_size_.height = 0;
  focal_length_ = 0;
  fNear_ = .1;
  fFar_ = 1000;
  initialized_ = false;
}

CameraHelper::CameraHelper(string filename){
  initialized_ = false;
  /*
    horizontal field of view,
    vertical field of view,
    focal length (assuming 1.0f for both apertureWidth and apertureHeight),
    principal point,
    aspect ratio parameters fx and fy,
    distortion parameters k1, k2, p1, and p2
  */

  FileStorage fs2(filename, FileStorage::READ);
  if(!fs2.isOpened()){
	  printf("Error, cannot find camera settings.");
	  return;
  }
  vector<int> imageSize;

  fs2["imageSize"] >> imageSize;
  fs2["cameraMatrix"] >> intrinsic_;
  fs2["distCoeffs"] >> distortion_coeffs_;

  fs2.release();

  img_size_.width = imageSize.at(0);
  img_size_.height = imageSize.at(1);

  fNear_ = 0.1;
  fFar_ = 1000;
  double ratio = img_size_.width*1.0f/img_size_.height;
  Point2d pp;
  calibrationMatrixValues(intrinsic_,img_size_,ratio,1.0f,fovx_,fovy_,focal_length_,pp,aspect_ratio);

  initialized_ = true;
  fprintf( stderr, "Camera initialized.\n" );
}

void CameraHelper::calibrateFromImages(const vector<Mat>& image, const int board_w, const int board_h){

  //set the img_width and img_height private params
  img_size_.width = image[0].size().width;
  img_size_.height = image[0].size().height;

  Size board_size = Size(board_w, board_h);

  vector<Point2f> corners; //locations
  vector< vector<Point3f> > object_points; //3D points in world-space
  vector< vector<Point2f> > image_points; //2D points in image-space
  vector<Mat> rvecs;
  vector<Mat> tvecs;


  //for each image
  for(unsigned int i = 0; i < image.size(); i++){
    //Find Chessboard Corners
    bool found = findChessboardCorners(image[i], board_size, corners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

    if(found){
		Mat gray_image;
		cvtColor(image[i], gray_image, CV_RGB2GRAY);
		cornerSubPix(gray_image, corners, Size(11, 11), Size(-1, -1),
				TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
        //create 3D world-space points for chessboard corners
        vector<Point3f> obj;
        for(int j = 0; j < board_w*board_h; j++)
          obj.push_back(Point3f(j/board_w, j%board_w, 0.0f));

        image_points.push_back(corners);
        object_points.push_back(obj);
    }
  }

  //set aspect ratios to 1
//  intrinsic_.ptr<double>(0)[0] = 1;
//  intrinsic_.ptr<double>(1)[1] = 1;

  //calibrate
  calibrateCamera(object_points, image_points, image[0].size(), intrinsic_, distortion_coeffs_, rvecs, tvecs);

  Point2d pp;
  double aspect_ratio;
  calibrationMatrixValues(intrinsic_, image[0].size(), 1.0, 1.0, fovx_, fovy_, focal_length_, pp, aspect_ratio);

  initialized_ = true;
}

void CameraHelper::getGLViewportArray(int v_array[4]){
  if(!initialized_)
    return;
//  v_array[0] = (int)(intrinsic_.at<double>(0,2) - img_size_.width/2.0);
  v_array[0] = 0;
  v_array[1] = 0;
//  v_array[1] = (int)(img_size_.height/2.0 - intrinsic_.at<double>(1,2));
  v_array[2] = img_size_.width;
  v_array[3] = img_size_.height;

}

double CameraHelper::getFy(){
	return intrinsic_.at<double>(1,1);
}

void CameraHelper::getGluPerspectiveArray(double glu_array[4]){
  if(!initialized_){
    //throw error
    return;
  }
  glu_array[0] = fovy_;
  glu_array[1] = (double)img_size_.width/img_size_.height;
  glu_array[2] = fNear_;
  glu_array[3] = fFar_;
}

void CameraHelper::undistortImage(Mat& image, Mat& corrected_image){
  if(!initialized_)
    return;
  undistort(image, corrected_image, intrinsic_, distortion_coeffs_);
}

void CameraHelper::setFNearFFar(const double fNear, const double fFar){
  fNear_ = fNear;
  fFar_ = fFar;
}

void CameraHelper::printCameraParameters(){
  if(!initialized_)
    return;
  cout << "width, height = " << img_size_.width << ", " << img_size_.height << endl;
  cout << "fx, fy = " << intrinsic_.at<double>(0,0) << ", " << intrinsic_.at<double>(1,1) << endl;
  cout << "cx, cy = " << intrinsic_.at<double>(0,2) << ", " << intrinsic_.at<double>(1,2) << endl;
  cout << "fovx, fovy = " << fovx_ << ", " << fovy_ << endl;
  cout << "focal length = " << focal_length_ << endl;
}

void CameraHelper::outputCameraParameters(string filename){
  if(!initialized_)
    return;

  FileStorage fs(filename, FileStorage::WRITE);

  time_t rawtime; time(&rawtime);
  fs << "calibrationDate" << asctime(localtime(&rawtime));
  fs << "imageSize" << img_size_;
  fs << "fovx" << fovx_;
  fs << "fovy" << fovy_;
  fs << "focalLength" << focal_length_;
  fs << "cameraMatrix" << intrinsic_;
  fs << "distCoeffs" << distortion_coeffs_;

  fs.release();
}
