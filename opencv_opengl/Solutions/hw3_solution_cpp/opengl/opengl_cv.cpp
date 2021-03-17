// Skeleton Code for CS290I Homework 1
// 2012, Jon Ventura and Chris Sweeney

// adapt the include statements for your system:

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <chrono>
#include <thread>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <cstdio>
#include <vector>
#include <math.h>

// IF DEBUG is defined, print tons of debug output.
#ifdef DEBUG
static bool debug = true;
#else
static bool debug = false;
#endif

// Must match the dimensions of the filmed calibration pattern.
#define GRID_WIDTH 8
#define GRID_HEIGHT 6
#define DISTORTION_CORRECTION 0
#define SLOWDOWN 0

using namespace std;

cv::VideoCapture *cap = NULL;
int width = 640;
int height = 480;
cv::Mat image;
cv::Mat distCoeffs;
int frame_counter = 0;
int g_nr_frames = 0;

// Intrinsic camera parameters.
static float fovx, fovy, focalLength, principalX, principalY, fx, fy, k1, k2, p1, p2;
cv::Mat cameraMat;

// Current AR overlay.
int overlay = 0;

// a useful function for displaying your coordinate system
void drawAxes(float length)
{
  //  return;  
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


bool getImagePoints(cv::Mat& image, int width, int height, vector<cv::Point2f>& corners) {
  cv::Size patternSize(width, height);
  bool boardFound = cv::findChessboardCorners(image, patternSize, corners, 0);
  return boardFound;
}


void createObjectPoints(int width, int height, vector<cv::Point3f>& points) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      points.push_back(cv::Point3f(x, y, 0));
    }
  }
}
// (0,0),(1,0),(2,0)...(9,0),(0,1)...(9,1)(0,2)... ...(9,6)

void display()
{
  if (SLOWDOWN) std::this_thread::sleep_for(std::chrono::milliseconds(SLOWDOWN));

  if (!image.data) {
    // This randomly occurs at the beginning of a video file, and always at its end.
    glutPostRedisplay();
    return;
  }

  int width = image.size().width;
  int height = image.size().height;

  cv::Mat tempimage;
  cv::flip(image, tempimage, 0);
  glViewport(0, 0, tempimage.size().width, tempimage.size().height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set projection matrix using intrinsic camera params.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-principalX / fx, (width - principalX) / fy, (principalY - height) / fy, principalY / fy, 1, 500);  
  
  // Locate calibration pattern.
  vector<cv::Point3f> objectPoints;
  vector<cv::Point2f> imagePoints;
  createObjectPoints(GRID_WIDTH, GRID_HEIGHT, objectPoints);
  bool found = getImagePoints(image, GRID_WIDTH, GRID_HEIGHT, imagePoints);
  if (!found) {
    // We couldn't find the (entire) pattern, so we can't determine the extrinsic parameters for this frame.
    printf("%d points not found \n", GRID_WIDTH * GRID_HEIGHT - imagePoints.size());
    cv::flip(image, tempimage, 0);
    glDisable(GL_DEPTH_TEST);
    glDrawPixels( tempimage.size().width, tempimage.size().height, GL_BGR, GL_UNSIGNED_BYTE, tempimage.ptr() );
    glEnable(GL_DEPTH_TEST);
    glutSwapBuffers();
    glutPostRedisplay();
    return;
  }

  // Solve for extrinsic parameters.
  cv::Mat rvec, tvec;
  cv::solvePnP(objectPoints, imagePoints, cameraMat, distCoeffs, rvec, tvec);
  cv::Mat undistorted;
  cv::undistort(image, undistorted, cameraMat, distCoeffs);

  // Draw points projected with OpenCV. Useful for verifying intrinsic/extrinsic parameters 
  // and debugging the OpenGL projection matrix.
  if (debug) {
    vector<cv::Point2f> projected;
    cv::projectPoints(objectPoints, rvec, tvec, cameraMat, distCoeffs, projected);
    for (int i = 0; i < projected.size(); i++) {
      cv::Point2f pt = projected.at(i);
      cv::circle(image, pt, 5, cv::Scalar(0, 0, 255));
    }
  }

  if (DISTORTION_CORRECTION)
    {
      // use undistorted camera image: 
      cv::flip(undistorted, tempimage, 0);
    } else 
    {
      // use original camera image:
      cv::flip(image, tempimage, 0);
    }
    
  glDisable(GL_DEPTH_TEST);
  glDrawPixels(tempimage.size().width, tempimage.size().height, GL_BGR, GL_UNSIGNED_BYTE, tempimage.ptr());
  glEnable(GL_DEPTH_TEST);

  // Convert rvec, tvec (from solvePnP) to transformation matrix.
  cv::Mat rm = cv::Mat(3, 3, CV_64F);
  cv::Rodrigues(rvec, rm); // Convert rotation vector to rotation matrix.
  // Top-left 3x3 = the rotation matrix. Top-right 3x1 = translation vector.
  const double xform[16] = {
    rm.at<double>(0,0), rm.at<double>(0,1), rm.at<double>(0,2), tvec.at<double>(0),
    rm.at<double>(1,0), rm.at<double>(1,1), rm.at<double>(1,2), tvec.at<double>(1),
    rm.at<double>(2,0), rm.at<double>(2,1), rm.at<double>(2,2), tvec.at<double>(2),
    0                 , 0                 , 0                 , 1
  };
  
  // Print the transformation matrix.
  if (debug) {
    printf("------------------------\n");
    for(int y = 0; y < 4; y++) {
      printf("|%f\t%f\t%f\t%f|\n", xform[4*y+0],xform[4*y+1],xform[4*y+2],xform[4*y+3]);
    }
    printf("------------------------\n");
  }    
 
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glScalef(1.0,-1.0, -1.0); // Welcome to the other hand...

  glMultTransposeMatrixd(xform); // Transpose, because we constucted xform row-major above.
 

  // Draw everything.
  float lightPos[] = {50, 50, -50, 0};
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

  drawAxes(1.0);

  switch (overlay) {
  case 0:
    glPushMatrix();
    if (debug) {
      // Draw a ground plane to help position the teapot.
      glDisable(GL_LIGHTING);
      glBegin(GL_QUADS);
      glVertex3f(10,10,0);
      glVertex3f(0,10,0);
      glVertex3f(0,0,0);
      glVertex3f(10,0,0);
      glEnd();
      glEnable(GL_LIGHTING);
    }
    glTranslatef(GRID_WIDTH/2, GRID_HEIGHT/2, -1.5);
    glRotatef(-90, 1, 0, 0);
    glutSolidTeapot(1.5);
    glPopMatrix();
    break;

  case 1:
    for(int i = 0; i < objectPoints.size(); i++) {
      cv::Point3f pt = objectPoints.at(i);
      glPushMatrix();
      glTranslatef(pt.x, pt.y, 0);
      glutSolidSphere(.3, 10, 10);
      glPopMatrix();
    }
    break;
  }

  // show the rendering on the screen
  glutSwapBuffers();

  // post the next redisplay
  glutPostRedisplay();
  
}

void reshape(int w, int h) {
  // set OpenGL viewport (drawable area)
  glViewport( 0, 0, w, h );
}

void mouse(int button, int state, int x, int y) {
}

void keyboard(unsigned char key, int x, int y) {
  switch ( key )
    {
    case 'q':
      // quit when q is pressed
      exit(0);
      break;

    case ' ':
      // Press [space] to alternate between overlays.
      overlay ^= 1;
      break;

    default:
      break;
    }
}

void idle() {
  // grab a frame from the camera
  (*cap) >> image;

  if (++frame_counter == g_nr_frames-1)
    {
      frame_counter = 0;
      cap->set(cv::CAP_PROP_POS_FRAMES, 0);
    }

}

// MAIN Function
////////////////

int main(int argc, char **argv) {
  int w,h;

  FILE *params = stdin;
  // Or, from a file.
  // FILE *params = fopen("params.txt", "r");

  if ( argc == 1 ) {
    // start video capture from camera
    cap = new cv::VideoCapture(0);
  } else if ( argc == 2 ) {
    // start video capture from file
    printf("From file\n");
    cap = new cv::VideoCapture(argv[1]);
  } else {
    fprintf( stderr, "usage: %s [<filename>]\n", argv[0] );
    return 1;
  }

  // check that video is opened
  if ( cap == NULL || !cap->isOpened() ) {
    fprintf( stderr, "could not start video capture\n" );
    return 1;
  }

  g_nr_frames = cap->get(cv::CAP_PROP_FRAME_COUNT);
    
  printf("Reading intrinsic parameters from STDIN...\n");

  // Load intrinsic parameters.
  fscanf(params, "%f\n", &fovx);
  fscanf(params, "%f\n", &fovy);
  fscanf(params, "%f\n", &focalLength);
  fscanf(params, "%f\n", &principalX);
  fscanf(params, "%f\n", &principalY);
  fscanf(params, "%f\n", &fx);
  fscanf(params, "%f\n", &fy);
  fscanf(params, "%f\n", &k1);
  fscanf(params, "%f\n", &k2);
  fscanf(params, "%f\n", &p1);
  fscanf(params, "%f\n", &p2);

  // Create a camera matrix.
  float m[] = {fx, 0, principalX, 
	       0, fy, principalY, 
	       0, 0, 1};
  cameraMat = cv::Mat(3, 3, CV_32F, m);

  // And the distortion coefficients matrix.
  float d[] = {k1, k2, p1, p2};
  distCoeffs = cv::Mat(1, 4, CV_32F, d);

  // get width and height
  w = (int) cap->get( cv::CAP_PROP_FRAME_WIDTH );
  h = (int) cap->get( cv::CAP_PROP_FRAME_HEIGHT );
  // On Linux, there is currently a bug in OpenCV that returns 
  // zero for both width and height here (at least for video from file)
  // hence the following override to global variable defaults: 
  width = w ? w : width;
  height = h ? h : height;

  // initialize GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowPosition(20, 20);
  glutInitWindowSize(width, height);
  glutCreateWindow("CS290I Assignment 1");

  // Initialize GL.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glDepthFunc(GL_LESS);
  glShadeModel(GL_FLAT);
  glDisable(GL_CULL_FACE); // Nicer looking teapots.
  glClearColor(0.0f, 0.0f, 0.0f, 1);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0f);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  // set up GUI callback functions
  glutDisplayFunc( display );
  glutReshapeFunc( reshape );
  glutMouseFunc( mouse );
  glutKeyboardFunc( keyboard );
  glutIdleFunc( idle );

  // start GUI loop
  glutMainLoop();

  return 0;
}
