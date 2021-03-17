// Skeleton Code for CS290I Homework 1
// 2012, Jon Ventura and Chris Sweeney

// adapt the include statements for your system:

#include <unistd.h>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <chrono>
#include <thread>


#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <cstdio>
#include <vector>
#include <math.h>

#define CLASSIFIER_PATH "classifier.xml"
#define RAD_TO_DEG 57.2957795

using namespace std;

cv::VideoCapture *cap = NULL;
double width = 1280;
double height = 720;
cv::Mat image;
cv::Mat distCoeffs;
cv::CascadeClassifier classifier;
cv::Rect faceRect;
double faceX, faceY, faceZ, faceX2, faceY2;
bool backgroundCaptureQueued = false;
bool useSubtractionMethod = false;
cv::Mat background;
cv::Mat prevGrayscaled;

// Intrinsic camera parameters.
static float fovx, fovy, focalLength, principalX, principalY, fx, fy, k1, k2, p1, p2;
cv::Mat cameraMat;

// Intrinsic head parameters.
static float faceHeight = 15;

// Current AR overlay.
int overlay = 0;
float rot = 0;
bool showRectangle = false;
bool showMotion = false;
bool showObject = true;

// Buttons. Left/right are actually mirrored.
cv::Rect leftButton = cv::Rect(20, 20, 50, 50);
cv::Rect rightButton = cv::Rect(width - 70, 20, 50, 50);

//
int buttonThreshold = 100;
bool leftPressed = false;
bool rightPressed = false;

void display()
{
  if (!image.data) {
    // This randomly occurs at the beginning of a video file, and always at its end.
    glutPostRedisplay();
    return;
  }

  int width = image.size().width;
  int height = image.size().height;

  cv::Mat tempimage;
  cv::flip(image, tempimage, 0);
  glViewport(0, 0, width, height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set projection matrix using intrinsic camera params.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-principalX / fx, (width - principalX) / fy, (principalY - height) / fy, principalY / fy, 1, 3000);
  //gluPerspective(fovy, width/height, 1, 1000);

  // 2D drawing via OpenCV.
  // -----------
  cv::Mat undistorted, mirrored;
  cv::undistort(image, undistorted, cameraMat, distCoeffs);

  // Draw buttons.
  cv::Scalar yellow(0, 255, 255);
  cv::rectangle(undistorted, leftButton, leftPressed ? yellow : cv::Scalar(0, 255, 0),
		cv::FILLED);
  cv::rectangle(undistorted, rightButton, rightPressed ? yellow : cv::Scalar(0, 0, 255),
		cv::FILLED);


  // Draw face region.
  if (showRectangle) {
    cv::rectangle(undistorted, faceRect, cv::Scalar(0, 255, 255));
  }

  cv::flip(undistorted, tempimage, 0);
  cv::flip(tempimage, mirrored, 1);
  glDisable(GL_DEPTH_TEST);
  glDrawPixels(tempimage.size().width, tempimage.size().height, GL_BGR, GL_UNSIGNED_BYTE, mirrored.ptr());
  glEnable(GL_DEPTH_TEST);

  // 3D drawing via OpenGL.
  // -----------
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glScalef(-1, 1, 1); // Mirror the X axis to match the mirrored video.

  // Draw points at the corners of the rectangle.
  //if (showRectangle) {
  //  glDisable(GL_LIGHTING);
  //  glColor3f(1,.5,.2);
  //  glBegin(GL_POINTS);
  //  glVertex3d(faceX, faceY, faceZ);
  //  glColor3f(1,0,0);
  //  glVertex3d(faceX2, faceY, faceZ);
  //  glVertex3d(faceX2, faceY2, faceZ);
  //  glVertex3d(faceX, faceY2, faceZ);
  //  glEnd();
  //   glEnable(GL_LIGHTING);
  //}

  // Mask the head.
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glBegin(GL_QUADS);
    glVertex3d(faceX, faceY, faceZ);
    glVertex3d(faceX2, faceY, faceZ);
    glVertex3d(faceX2, faceY2, faceZ);
    glVertex3d(faceX, faceY2, faceZ);
  glEnd();
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  // Draw an object.
  float lightPos[] = {100, 100, 100, 0};
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glTranslatef(faceX + (faceX2 - faceX ) /2, faceY + (faceY2 - faceY) / 2, faceZ);
  glColor3f(1, 1, 1);
  glRotatef(rot, 0, 1, 0);
  rot += 9.0f;
  glTranslatef(0, 0, faceHeight * .75);
  if (showObject) {
    switch (overlay) {
    case 0:
      glutSolidTeapot(4);
      break;
    case 1:
      glRotatef(-90, 1, 0, 0);
      glutSolidCone(2.5, 5, 10, 10);
      break;
    case 2:
      glutSolidTorus(1, 3, 20, 20);
      break;
    }
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

    case 'r':
      // Toggle image-space rectangle.
      showRectangle ^= true;
      break;

    case 'b':
      // Use the next captured frame as the (subtracted) background.
      backgroundCaptureQueued = true;
      break;

    case 'g':
      // Toggle motion detection display.
      showMotion ^= true;
      break;

    case 'f':
      // Toggle between optical flow and background subtraction.
      useSubtractionMethod ^= true;
      printf("\nCurrent motion detection method: %s\n",
	     useSubtractionMethod ? "Background Subtraction" : "Optical Flow");
      if (useSubtractionMethod && background.size().width == 0) {
	fprintf(stderr, "Error: Press 'b' to capture background\n");
      }
      break;

    case 'o':
      // Toggle object display.
      showObject ^= true;
      break;

    default:
      break;
    }
}

bool checkButton(cv::Rect& button, cv::Mat& imgMat, bool& toggle) {
  // Take the mean of the pixels within the button region of the grayscale image as an
  // easy way of counting them.
  cv::Rect scaledButton(button.x / 2, button.y / 2, button.width / 2, button.height / 2);
  unsigned char value = cv::mean(imgMat(scaledButton))[0];
  if (value > buttonThreshold) {
    if (!toggle) {
      toggle = true;
      return true;
    }
  } else {
    toggle = false;
  }
  return false;
}

void idle() {
  // grab a frame from the camera
  (*cap) >> image;

  if (image.data) {
    // Create a 1/2 size grayscale copy of the frame.
    cv::Mat scaled, grayscaled;
    cv::resize(image, scaled, cv::Size(width/2, height/2)); // Note: image assumed to be 640x480.
    cv::cvtColor(scaled, grayscaled, CV_RGB2GRAY);
    if (backgroundCaptureQueued) {
      backgroundCaptureQueued = false;
      background = grayscaled;
    }

    bool motion = false;
    cv::Mat diff, threshold;
    if (useSubtractionMethod) {
      // Background subtraction method.
      if (background.size() == grayscaled.size()) {
	cv::absdiff(grayscaled, background, diff);
	motion = true;
      }
    } else {
      // Optical flow method.
      if (prevGrayscaled.size() == grayscaled.size()) {
	cv::Mat flow;
	cv::calcOpticalFlowFarneback(prevGrayscaled, // Previous.
				     grayscaled,  // Next.
				     flow, // Flow output.
				     0.5, // Pyramid scale.
				     1, // Levels.
				     3, // Window size.
				     1, // Iterations.
				     5, // polyN.
				     1.1, // polySigma.
				     0); // Flags.

	// flow currently contains (x, y) vectors. Take the magnitude of each vector to make a
	// grayscale image.

	cv::Mat split[2], mag;
	cv::split(flow, split);
	cv::magnitude(split[0], split[1], mag);

	mag.convertTo(diff, CV_8U, 3);
	motion = true;
      }
      prevGrayscaled = grayscaled;
    }

    if (motion) {
      cv::threshold(diff, threshold, 10, 255, cv::THRESH_BINARY);

      // Test the buttons and switch objects if they are pressed.
      overlay += checkButton(leftButton, threshold, leftPressed) ? 1 :
	checkButton(rightButton, threshold, rightPressed) ? -1 : 0;
      if (overlay == -1) {
	overlay = 2;
      }
      overlay %= 3;

      // Show the motion image instead of the camera frame.
      // Note: we don't need to upscale the threshold image unless it's actually being displayed,
      // so the earlier button tests work with the downscaled image.
      if (showMotion) {
	// Prepare threshold image for display by upscaling and converting to RGB.
	cv::Mat rgb;
	cv::cvtColor(threshold, rgb, CV_GRAY2RGB);
	cv::resize(rgb, image, cv::Size(width, height)); // "image" is drawn in display().
      }
    }

    // Find a face.
    vector<cv::Rect> objects;
    classifier.detectMultiScale(grayscaled, objects, 1.1, 3,
				cv::CASCADE_SCALE_IMAGE | cv::CASCADE_FIND_BIGGEST_OBJECT);

    if (objects.size()) {
      // We need to scale the face position back to video resolution.
      faceRect = objects.at(0);
      faceRect.x *= 2;
      faceRect.y *= 2;
      faceRect.width *= 3;
      faceRect.height *= 2; // Scale height by an additional 1.5.
      
      //printf("%g\n",fy);
      //printf("%g\n\n",fx);

      // Calculate 3D coordinates of face. See README for details.
      // Top left corner of face.
      faceZ = -fy * faceHeight / faceRect.height;
      faceX = -faceZ * (faceRect.x - width/2) / (fy*width/height) - 10;
      faceY = faceZ * (faceRect.y - height/2) / fy;

      //printf("%g\n",faceX);
      //printf("%g\n",faceY);
      //printf("%g\n",faceZ);

      // Opposite corner.
      faceX2 = -faceZ * (faceRect.x + faceRect.width - width/2) / (fy*width/height);
      faceY2 = faceZ * (faceRect.y + faceRect.height - height/2) / fy;
    }
  }

}

int main(int argc, char **argv) {
  int w,h;

  FILE *params = stdin;
  // Or, from a file.
  // FILE *params = fopen("params.txt", "r");

  if ( argc == 1 ) {
    // start video capture from camera
    cap = new cv::VideoCapture(2);
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

  // We weren't explicitly given the FOV for the sample videos, so I didn't put it in the 
  // parameters file. Calculate it from the focal length here:
  fovx = 2 * atan(.5 * width / fx) * RAD_TO_DEG;
  fovy = 2 * atan(.5 * height / fy) * RAD_TO_DEG;

  // Create a camera matrix.
  float m[] = {fx, 0, principalX,
	       0, fy, principalY,
	       0, 0, 1};
  cameraMat = cv::Mat(3, 3, CV_32F, m);

  // And the distortion coefficients matrix.
  float d[] = {k1, k2, p1, p2};
  distCoeffs = cv::Mat(1, 4, CV_32F, d);

  // Create classifier.
  classifier = cv::CascadeClassifier(CLASSIFIER_PATH);
  if (classifier.empty()) {
    fprintf(stderr, "Classifier not loaded (attempted to use [%s]).\n", CLASSIFIER_PATH);
    assert(classifier.empty());
  }

  // get width and height
  w = (int) cap->get( cv::CAP_PROP_FRAME_WIDTH );
  h = (int) cap->get( cv::CAP_PROP_FRAME_HEIGHT );
  // On Linux, there is currently a bug in OpenCV that returns
  // zero for both width and height here (at least for video from file)
  // hence the following override to global variable defaults:
  width = w ? w : width;
  height = h ? h : height;

  printf("\nCurrent motion detection method: %s\n",
	 useSubtractionMethod ? "Background Subtraction" : "Optical Flow");

  // initialize GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowPosition(20, 20);
  glutInitWindowSize(width, height);
  glutCreateWindow("CS290I Assignment 2");

  // Initialize GL.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glDepthFunc(GL_LESS);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_CULL_FACE); // Nicer looking teapots.
  glClearColor(0.0f, 0.0f, 0.0f, 1);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0f);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_POINT_SMOOTH);
  glPointSize(10);

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
