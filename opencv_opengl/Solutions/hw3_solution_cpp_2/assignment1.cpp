// Sample Solution Code for CS290I Homework 1
// 2012, Chris Sweeney, 2013 Tobias Hollerer

// adapt the include statements for your system:

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>


#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <cstdio>
#include <iostream>
#include <fstream>
#include <time.h>
#include "cameraHelper.h"

using namespace std;
using namespace cv;

#define GRID_WIDTH 8
#define GRID_HEIGHT 6

VideoCapture *cap = NULL;
int image_width = 640;
int image_height = 480;
int frame_counter = 0;
int g_nr_frames = 0;


Mat image;
Mat flipped_image;
CameraHelper *camera = NULL;

int v_array[4];
double glu_array[4];
double model_array[16];

bool CALIBRATION_MODE = false;
vector<Mat> calibration_images;

bool AXIS_MODE = true;
int AR_MODE = 0;
const int AR_SPHERES = 0;
const int AR_TEAPOT = 1;
int countimgs = 0;

// a useful function for displaying your coordinate system
void DrawAxis(float length)
{
  glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT) ;

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) ;
  glDisable(GL_LIGHTING) ;

  glBegin(GL_LINES) ;
  glColor3f(1,0,0) ;
  glVertex3f(0,0,0) ;
  glVertex3f(length,0,0);

  glColor3f(0,1,0) ;
  glVertex3f(0,0,0) ;
  glVertex3f(0,length,0);

  glColor3f(0,0,1) ;
  glVertex3f(0,0,0) ;
  glVertex3f(0,0,length);
  glEnd() ;

  glPopAttrib() ;
}

void drawSpheres(){
  glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT) ;
  // Draw the spheres
  for(int i=0; i<GRID_HEIGHT; i++)
    {
      for(int j=0; j<GRID_WIDTH; j++)
        {
          glPushMatrix() ;
          glTranslatef(i, j, 0.0) ;
          //glTranslatef(j, i, 0.0) ;
          glColor3f(1,1,1) ;
          glutSolidSphere(0.3, 100, 100);
          glPopMatrix() ;
        }
    }
  glPopAttrib() ;
}

void drawTeapot(){
  //glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT) ;
  glPushMatrix();

  //glPolygonMode(GL_FRONT, GL_FILL) ;
  //glColor3f(1,1,1);
  glTranslatef(3.5, 5, 1.5);
  //glTranslatef(5, 3.5, 1.5);
  glRotatef(90, 1, 0, 0);
  glutSolidTeapot(1.5);

  glPopMatrix();
  //glPopAttrib() ;
}

void display()
{
  // clear the window
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // show the current camera frame
  glDisable(GL_DEPTH_TEST);
  glDrawPixels(flipped_image.size().width, flipped_image.size().height, GL_BGR, GL_UNSIGNED_BYTE, flipped_image.ptr() );
  glEnable(GL_DEPTH_TEST);

  if(!CALIBRATION_MODE){
    //////////////////////////////////////////////////////////////////////////////////
    // Here, set up new parameters to render a scene viewed from the camera.

    //set viewport

    glViewport(v_array[0], v_array[1], v_array[2], v_array[3]);
    //glViewport(0, 0, width, height);

    //set projection matrix using intrinsic camera params
    glMatrixMode(GL_PROJECTION);

    // set camera perspective params from intrinsic parameters
    glLoadIdentity();
    gluPerspective(glu_array[0], glu_array[1], glu_array[2], glu_array[3]);
    /* Alternative Way to set GL_PROJECTION matrix: 
    double principalX, principalY, fx, fy, fovx, fovy;
    if (camera->getIntrinsicParams(fx,fy,fovx,fovy,principalX,principalY))
      glFrustum(-principalX / fx, (width - principalX) / fy, (principalY - height) / fy, principalY / fy, 1, 1000);  
    else
      cout << "Didn't get Intrinsic parameters!" << endl;
    */

    // set modelview matrix using extrinsic camera params
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(1.0, -1.0, -1.0);
    glMultMatrixd(model_array);

    /////////////////////////////////////////////////////////////////////////////////
    // Drawing routine

    // Draw everything.
    float lightPos[] = {50, 50, 50, 0};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    if(AXIS_MODE == 1)
      DrawAxis(100);

    if(AR_MODE == AR_SPHERES)
      drawSpheres();
    else if (AR_MODE == AR_TEAPOT)
      drawTeapot();
  }
  // show the rendering on the screen
  glutSwapBuffers();

  // post the next redisplay
  glutPostRedisplay();

}

void reshape( int w, int h )
{
  // set OpenGL viewport (drawable area)
  glViewport( 0, 0, w, h );
}

void mouse( int button, int state, int x, int y )
{
  if ( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {

    }
}

void keyboard( unsigned char key, int x, int y )
{
  switch ( key )
    {
    case 'q':
      // quit when q is pressed
      exit(0);
      break;
    case 'a':
      AXIS_MODE = !AXIS_MODE;
      break;
    case ' ':
      AR_MODE = (AR_MODE+1)%2;
      break;
    case 'c':
      if(CALIBRATION_MODE){
	vector<Point2f> corners;
	bool found = findChessboardCorners(image, Size(GRID_WIDTH,GRID_HEIGHT), corners, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FILTER_QUADS + cv::CALIB_CB_FAST_CHECK + cv::CALIB_CB_NORMALIZE_IMAGE);
	if(found){
	  cout << "calibration image captured" << endl;
	  calibration_images.push_back(image);
	  if (countimgs < 10)
	    imwrite( string("image0") + to_string(countimgs++) + string(".png"), image );
	  else
	    imwrite( string("image") + to_string(countimgs++) + string(".png"), image );
	}
	else{
	  cout << "image could not be used for calibration" << endl;
	}
      }
      default:
        break;
    }
}

/* It is usually good practice to let the display callback function do as little data processing
 * as possible. Any data/information processing should occur in the idle function, which is why
 * we calculate the necessary parameters in this stage.
 */
void idle()
{
  // grab a frame from the camera
  (*cap) >> image;
  //cout << frame_counter << " " << g_nr_frames << endl;
 
  // If the last frame is reached, reset the capture and the frame_counter
  if (++frame_counter == g_nr_frames-10)
    {
      frame_counter = 0; 
      cap->set(cv::CAP_PROP_POS_FRAMES, 0);
    }

  Mat corrected_image;

  // CALIBRATION_MODE means video calibration is occuring
  // spend the first few seconds gathering frames to use for calibration
  if(CALIBRATION_MODE){
    flip(image, flipped_image, 0);
    if(calibration_images.size() > 10){
      camera->calibrateFromImages(calibration_images, 10, 7);
      CALIBRATION_MODE = false;
    }
  }
  else{
    camera->getGLViewportArray(v_array);

    camera->getGluPerspectiveArray(glu_array);
    cout << "fovy: " << glu_array[0] << endl;

    Mat rvec, tvec;
    double reproj_error = camera->getExtrinsicParams(image, rvec, tvec);
    cout << "Reprojection Error: " << reproj_error << endl;
  
    if(reproj_error != -1.0)
      CameraHelper::convertToGLModelviewArray(rvec, tvec, model_array);

    camera->undistortImage(image, corrected_image);
    flip(corrected_image, flipped_image, 0);
  }
}

// reads in a text file that either contains camera parameters in the HW1 format, or the names of images that are to be used
// for camera calibration
void initFromTextFile(char* in_file){
  //calibrate camera parameters
  vector<Mat> images;
  string img_filename;
  ifstream config_file(in_file);
  int c = 0; 

  if(config_file.is_open()){
    if ((c=config_file.peek()) >='0' && c<='9')
      {
	// this is a camera parameter file
	config_file.close();
	camera = new CameraHelper(in_file, GRID_WIDTH, GRID_HEIGHT,image_width,image_height);
	cerr << "INIT FROM FILE THAT CONTAINS CAMERA PARAMETERS!" << endl;
      }
    else
      {
	// this is a file listing calibration images 	
	camera = new CameraHelper();
	while (config_file.good()){
	  config_file >> img_filename;
	  images.push_back(imread(img_filename.c_str()));
	}
	camera->calibrateFromImages(images, GRID_WIDTH, GRID_HEIGHT); // 10,7
	cerr << "INIT FROM FILE THAT NAMES CALIBRATION IMAGES!" << endl;
	config_file.close();
      }
  }
}


int main( int argc, char **argv )
{
  if ( argc <= 2 ) {
    // start video capture from camera
    cap = new VideoCapture(0);
    cout << "Live camera with " << (g_nr_frames = cap->get(cv::CAP_PROP_FRAME_COUNT)) << " frames!" << endl;
  } else if ( argc == 3 ) {
    // start video capture from file
    cap = new VideoCapture(argv[2]);
    cout << "Video File with " << (g_nr_frames = cap->get(cv::CAP_PROP_FRAME_COUNT)) << " frames!" << endl;

  } else {
    fprintf( stderr, "use in one of the following ways:\n %s\n %s <img list filename>\n %s <img list filename> <input video filename>\n **Note** img list filename should be the name of a text file that includes the filenames of images used for calibration (one per line)\n", argv[0], argv[0], argv[0] );
    return 1;
  }

  // if no arguments are passed, then calibrate with the video feed
  if(argc == 1)
    CALIBRATION_MODE = true;

  // check that video is opened
  if ( cap == NULL || !cap->isOpened() ) {
    fprintf( stderr, "could not start video capture\n" );
    return 1;
  }

  // get width and height
  int w = (int) cap->get( cv::CAP_PROP_FRAME_WIDTH );
  int h = (int) cap->get( cv::CAP_PROP_FRAME_HEIGHT );
  cout << "width: " << w << ", height: " << h;
  // On Linux, there is currently a bug in OpenCV that returns
  // zero for both width and height here (at least for video from file)
  // hence the following override to global variable defaults:
  image_width = w ? w : image_width;
  image_height = h ? h : image_height;

  //initialize camera params
  if(!CALIBRATION_MODE)
   initFromTextFile(argv[1]);
  else 
    camera = new CameraHelper();

  // initialize GLUT
  glutInit( &argc, argv );
  glutInitDisplayMode( GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE );
  glutInitWindowPosition( 20, 20 );
  glutInitWindowSize( image_width, image_height );
  glutCreateWindow( "OpenGL / OpenCV Example" );

  // set up GUI callback functions
  glutDisplayFunc( display );
  glutReshapeFunc( reshape );
  glutMouseFunc( mouse );
  glutKeyboardFunc( keyboard );
  glutIdleFunc( idle );

  // Initialize GL.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glDepthFunc(GL_LESS);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_CULL_FACE); // See the inside of the teapot through the gaps 
  glClearColor(0.0f, 0.0f, 0.0f, 1);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0f);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  // glEnable(GL_COLOR_MATERIAL);
  // glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  
  // start GUI loop
  glutMainLoop();

  return 0;
}
