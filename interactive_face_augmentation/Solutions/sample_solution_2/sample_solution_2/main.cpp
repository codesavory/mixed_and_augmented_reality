// Sample Solution Code for CS290I Homework 1
// 2012, Chris Sweeney

// adapt the include statements for your system:

#include <opencv/cv.h>
#include <opencv/highgui.h>

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
#include <iostream>
#include <fstream>
#include <time.h>
#include "cameraHelper.h"

using namespace std;
using namespace cv;

string CALIBRATION_LOCATION = "./intrinsic.yaml";
string CASCADE_FILE = "./haarcascade_frontalface_default.xml";
//string CASCADE_FILE = "./haarcascade_frontalface_alt2.xml";
const double HEAD_HEIGHT_CM = 15;
const double RECTANGLE_HEIGHT_SCALE = 1;

int rotation = 0;

VideoCapture *cap = NULL;
int width = 640;
int height = 480;
Mat image;
Mat background;
Mat imageSegmentation;
Mat prevImg;
CameraHelper *camera = NULL;
Rect backButton, nextButton;
bool backSelected = false;
bool nextSelected = false;

CascadeClassifier cascade;

int v_array[4];
double glu_array[4];
double model_array[16];
Point3f faceCenter;

bool CALIBRATION_MODE = false;
vector<Mat> calibration_images;

Rect rect;

bool AXIS_MODE = false;
int DISPLAY_MODE = 0;
int AR_MODE = 0;
const int AR_SPHERES = 0;
const int AR_TEAPOT = 1;
int RECTANGLE_ON = 0;

void drawRectangle() {
	rectangle(image, rect, Scalar(100, 50, 200));
	circle(image, Point(rect.x, rect.y), 20, Scalar(100, 50, 200));
}

void drawButtons() {
	Scalar back = Scalar(34, 34,201);
	Scalar next = Scalar(32, 191, 17);
	Scalar selected = Scalar(0, 255, 255);

	if(nextSelected){
		next = selected;
	}
	if(backSelected){
		back = selected;
	}

	rectangle(image, backButton,back,-1);
	rectangle(image, nextButton,next,-1);
	rectangle(imageSegmentation, backButton,back,-1);
	rectangle(imageSegmentation, nextButton,next,-1);
}

// a useful function for displaying your coordinate system
void DrawAxis(float length) {
	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);

	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(length, 0, 0);

	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, length, 0);

	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, length);
	glEnd();

	glPopAttrib();
}

void drawTeapot() {
	glRotatef(rotation * 4, 0, 1, 0);
	glutSolidTeapot(5.0);

}

void drawCylinder() {
	glRotatef(-90, 1, 0, 0);
	glBegin(GL_QUADS);
	GLUquadric* quad = gluNewQuadric();
	gluQuadricDrawStyle(quad, GLU_LINE);
	gluCylinder(quad, 5, 0, 20, 100, 100);
	glEnd();
}

void drawObjects() {

	glColorMask(false, false, false, false);
	glPushMatrix();
	glTranslatef(faceCenter.x, faceCenter.y, faceCenter.z);
	glRectf(-5.5, -10.5, 5.5, 20);
	glColorMask(true, true, true, true);

	glPopMatrix();
	glPushMatrix();

	glDepthFunc(GL_LESS);
	glColor3f((rand() * 1.0f) / RAND_MAX, (rand() * 1.0f) / RAND_MAX,
			(rand() * 1.0f) / RAND_MAX);
	glTranslatef(faceCenter.x, faceCenter.y, faceCenter.z);
	glTranslatef(sin(rotation * 1.0f / 10) * 20, 0,
			cos(rotation * 1.0f / 10) * 20);
	switch (AR_MODE) {
	case 0:
		drawTeapot();
		break;
	case 1:
		drawCylinder();
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
	rotation++;

	glPopMatrix();

}

void drawShperes() {
	glPushMatrix();
	double offset = -HEAD_HEIGHT_CM / 2;
	glTranslatef(offset, offset, 0);
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 2; ++j) {
			glPushMatrix();
			glTranslatef(faceCenter.x + HEAD_HEIGHT_CM * i,
					faceCenter.y + HEAD_HEIGHT_CM * j, faceCenter.z);
			glColor3d(.5, 0, 0);
			glutSolidSphere(1.5, 100, 100);
			glPopMatrix();
		}
	}
	glPopMatrix();
}

void display() {
	// clear the window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!CALIBRATION_MODE) {

		if (RECTANGLE_ON != 2) {
			drawRectangle();
		}
		drawButtons();

		// show the current camera frame
		if(DISPLAY_MODE != 0){
			glDrawPixels(imageSegmentation.cols, imageSegmentation.rows, GL_LUMINANCE,
					GL_UNSIGNED_BYTE, imageSegmentation.ptr());
		} else {
			glDrawPixels(image.cols, image.rows, GL_BGR,
					GL_UNSIGNED_BYTE, image.ptr());
		}

		//////////////////////////////////////////////////////////////////////////////////
		// Here, set up new parameters to render a scene viewed from the camera.

		//set viewport

		glViewport(v_array[0], v_array[1], v_array[2], v_array[3]);

		//set projection matrix using intrinsic camera params
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// set camera perspective params from intrinsic parameters

		gluPerspective(glu_array[0], glu_array[1], glu_array[2], glu_array[3]);
		glScalef(1.0, 1.0, -1.0);

		// set modelview matrix using extrinsic camera params
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		/////////////////////////////////////////////////////////////////////////////////
		// Drawing routine

		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

		glEnable(GL_LIGHT0);
		GLfloat pos[4] = { 15, 15, 5, 1 };
		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glEnable(GL_DEPTH_TEST);
		drawObjects();
		if (RECTANGLE_ON == 1) {
			drawShperes();
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT0);
	}

	// show the rendering on the screen
	glutSwapBuffers();

	// post the next redisplay
	glutPostRedisplay();

}

void reshape(int w, int h) {
	// set OpenGL viewport (drawable area)
	glViewport(0, 0, w, h);
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {

	}
}

void captureBackground(){
	Mat unflipped_image;
	(*cap) >> unflipped_image;
	Mat capture;
	flip(unflipped_image, capture, 0);

	// Scale image
	Mat resize_image;
	int height = 320 * image.rows / image.cols;
	resize(capture, resize_image, Size(320, height));

	// Make image gray
	Mat gray_image;
	cvtColor(resize_image, background, CV_RGB2GRAY);
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'q':
		// quit when q is pressed
		exit(0);
		break;
	case 'b':
		captureBackground();
		break;
	case 'r':
		RECTANGLE_ON = (RECTANGLE_ON + 1) % 3;
		break;
	case 'o':
		AR_MODE = (AR_MODE + 1) % 3;
		break;
	case 'g':
		DISPLAY_MODE = (DISPLAY_MODE + 1) % 3;
		break;
	case 'c':
		if (CALIBRATION_MODE) {
			vector<Point2f> corners;
			bool found = findChessboardCorners(image, Size(10, 7), corners,
					CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
			if (found) {
				cout << "calibration image captured" << endl;
				calibration_images.push_back(image);
			} else {
				cout << "image could not be used for calibration" << endl;
			}
		}
		break;
	default:
		break;
	}
}

void calibrateImageVector() {
	camera->calibrateFromImages(calibration_images, 10, 7);
	camera->outputCameraParameters(CALIBRATION_LOCATION);
}

void renderSelectionByOF(const Mat gray_image){
	Mat flow;
	Mat flowMap(gray_image.rows, gray_image.cols, gray_image.type());
	calcOpticalFlowFarneback(prevImg,gray_image,flow,.5,1,3,1,5,1.1,0);
	for (int x = 0; x < flow.cols; ++x) {
		for (int y = 0; y < flow.rows; ++y) {
			flowMap.at<char>(y,x) = sqrt(pow(flow.at<Point2f>(y,x).x,2)+pow(flow.at<Point2f>(y,x).y,2));
		}
	}
	Mat thresh;
	resize(flowMap, thresh, Size(image.cols, image.rows));
	threshold(thresh,imageSegmentation,20,255,THRESH_BINARY);
}

void renderSelectionByBS(const Mat gray_image){
	Mat difference;
	absdiff(gray_image,background,difference);
	Mat thresh;
	threshold(difference,thresh,40,255,THRESH_BINARY);
	resize(thresh, imageSegmentation, Size(image.cols, image.rows));
}

void initializeButtons(){
	backButton = Rect(image.cols*.02, image.rows*.8, image.cols*.12, image.cols*.12);
	nextButton = Rect(image.cols*.86,image.rows*.8,image.cols*.12,image.cols*.12);
}

void checkButtons(){
	Scalar sumNext = sum(imageSegmentation(nextButton));
	Scalar sumBack = sum(imageSegmentation(backButton));

	if(backSelected == false && sumBack.val[0] > 0){
		backSelected = true;
		AR_MODE = (AR_MODE - 1) % 3;
	} else if (backSelected == true && sumBack.val[0] == false){
		backSelected = false;
	}

	if(nextSelected == false && sumNext.val[0] > 0){
		nextSelected = true;
		AR_MODE = (AR_MODE + 1) % 3;
	} else if (nextSelected == true && sumNext.val[0] == false){
		nextSelected = false;
	}
}

/* It is usually good practice to let the display callback function do as little data processing
 * as possible. Any data/information processing should occur in the idle function, which is why
 * we calculate the necessary parameters in this stage.
 */
void idle() {
	// grab a frame from the camera
	Mat unflipped_image;
	(*cap) >> unflipped_image;
	flip(unflipped_image, image, 0);

	if(background.datastart == NULL){
		captureBackground();
		initializeButtons();
	}

	// CALIBRATION_MODE means video calibration is occuring
	// spend the first few seconds gathering frames to use for calibration
	if (CALIBRATION_MODE) {
		if (calibration_images.size() > 10) {
			calibrateImageVector();
			CALIBRATION_MODE = false;
		}
	} else {

		// Scale image
		Mat resize_image;
		int height = 320 * image.rows / image.cols;
		resize(image, resize_image, Size(320, height));

		// Make image gray
		Mat gray_image;
		cvtColor(resize_image, gray_image, CV_RGB2GRAY);

		if(DISPLAY_MODE == 2){
			renderSelectionByOF(gray_image);
		} else {
			renderSelectionByBS(gray_image);
		}

		checkButtons();

		prevImg = gray_image;

		// Find faces
		vector<Rect> objects;
		cascade.detectMultiScale(gray_image, objects, 1.1, 2,
				CV_HAAR_SCALE_IMAGE | CV_HAAR_FIND_BIGGEST_OBJECT,
				Size(60, 60));

		// If faces found, update rectangle.
		if (objects.size() > 0) {
			double ratio = image.cols / 320.0;
			rect.width = round(objects.at(0).width * ratio);
			rect.height = round(
					objects.at(0).height * ratio * RECTANGLE_HEIGHT_SCALE);
			rect.y = round(objects.at(0).y * ratio - objects.at(0).height / 2);
			rect.x = round(objects.at(0).x * ratio);
		}

		// Calculate x,y to X,Y,Z, X and Y are the bottom left rectangle coordinates.
		double ratio = HEAD_HEIGHT_CM / rect.height;
		double fy = camera->getFy();
		double Z = fy * HEAD_HEIGHT_CM / rect.height;
		double X = (rect.x - image.cols / 2) * ratio + HEAD_HEIGHT_CM / 2;
		double Y = (rect.y - image.rows / 2) * ratio + HEAD_HEIGHT_CM / 2;

		faceCenter.x = X;
		faceCenter.y = Y;
		faceCenter.z = Z;

	}
}

// reads in a text file that contains the names of images that are to be used
// for camera calibration. Then saves that calibration.
void initFromTextFile(char* in_file) {
	//calibrate camera parameters
	vector<Mat> images;
	string img_filename;
	ifstream img_files(in_file);
	if (img_files.is_open()) {
		while (img_files.good()) {
			img_files >> img_filename;
			calibration_images.push_back(imread(img_filename.c_str()));
		}
	}
	calibrateImageVector();
}

int main(int argc, char *argv[]) {
	int w, h;
	// Calibrate camera and save the calibration file.
	if (argc == 3 && string(argv[1]) == "-c") {
		fprintf(stderr, "Calibration starting. File list: %s.\n", argv[2]);
		camera = new CameraHelper();
		initFromTextFile(argv[2]);
		fprintf(stderr, "Calibration done!");
		return 0;
	}

	// Calibrate camera and save the calibration file.
	if (argc == 2 && string(argv[1]) == "-c") {
		CALIBRATION_MODE = true;
	}

	// Load calibration file.
	camera = new CameraHelper(CALIBRATION_LOCATION);
	camera->getGLViewportArray(v_array);
	camera->getGluPerspectiveArray(glu_array);

	if (argc == 1) {
		// start video capture from camera
		cap = new VideoCapture(0);
	} else if (argc == 2) {
		// start video capture from file
		cap = new VideoCapture(argv[1]);
	} else {
		fprintf(
				stderr,
				"use in one of the following ways:\n %s\n %s <img list filename>\n %s <img list filename> <input video filename>\n **Note** img list filename should be the name of a text file that includes the filenames of images used for calibration (one per line)\n",
				argv[0], argv[0], argv[0]);
		return 1;
	}

	// check that video is opened
	if (cap == NULL || !cap->isOpened()) {
		fprintf(stderr, "could not start video capture\n");
		return 1;
	}

	// get width and height
	w = (int) cap->get(CV_CAP_PROP_FRAME_WIDTH);
	h = (int) cap->get(CV_CAP_PROP_FRAME_HEIGHT);
	// On Linux, there is currently a bug in OpenCV that returns
	// zero for both width and height here (at least for video from file)
	// hence the following override to global variable defaults:
	width = w ? w : width;
	height = h ? h : height;

	// Load the classification file.
	cascade.load(CASCADE_FILE);

	// initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(20, 20);
	glutInitWindowSize(width, height);
	glutCreateWindow("Homework 2, bitches.");

	// set up GUI callback functions
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	// start GUI loop
	glutMainLoop();

	return 0;
}
