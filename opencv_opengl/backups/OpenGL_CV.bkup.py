'''
this is a skeleton code to integrate OpenGL and OpenCV in python

it is a translation (with minor modifications) of the cpp code found here:
http://www.cs.ucsb.edu/~holl/CS291A/opengl_cv.cpp

installation of related libraries:
pip install opencv-contrib-python
pip install PyOpenGL PyOpenGL_accelerate

you can invoke this script in two ways:
python OpenGL_CV.py                  -----> captures from webcam
python OpenGL_CV.py <video-filename> -----> captures from the video file

2019, Nazmus Saquib
'''

import cv2
import numpy as np
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import sys
import math
#from scipy import linalg

#camera intrinsics
fx = 520.1784634338128
fy = 514.4133409181096
cx = 408.0818722075756
cy = 231.8592322296227

#distortion parameters
k1 = 0.028566500077574473
k2 = 0.002221520590884994
p1 = -0.0013725055266725207
p2 = 0.006616272424927662
k3 = -0.018437205730020462

board_width=8
board_height=6
# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((board_height*board_width,3), np.float32)
objp[:,:2] = np.mgrid[0:board_width,0:board_height].T.reshape(-1,2)
# termination criteria
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

dist = np.matrix([[ 0.0285665, 0.00222152, -0.00137251, 0.00661627, -0.01843721]])
mtx = np.matrix([[520.17846343, 0., 408.08187221],
[0., 514.41334092, 231.85923223],
[0., 0., 1.]])

def get_extrinsics(self, ipframe):
    gray = cv2.cvtColor(ipframe, cv2.COLOR_BGR2GRAY)
    ret, corners = cv2.findChessboardCorners(gray, (board_width, board_height), None)
    if ret == None:
        return None, _, _

    corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
    ret, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)

    return ret, rvecs, tvecs

class OpenGL_CV:

    def __init__(self, source):
        self.cap = cv2.VideoCapture(source)
        if self.cap.isOpened() == False:
            print("Error opening video stream...exiting")
            sys.exit(0)
        self.width = int(self.cap.get(3))
        self.height = int(self.cap.get(4))
        self.frame = None
        self.flipped_frame = None
        pass

    def draw_axes(self, length):
        '''a useful function to display your coordinate system'''
        glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
        glDisable(GL_LIGHTING)

        glBegin(GL_LINES)
        glColor3f(1,0,0)
        glVertex3f(0,0,0)
        glVertex3f(length,0,0)

        glColor3f(0,1,0)
        glVertex3f(0,0,0)
        glVertex3f(0,length,0)

        glColor3f(0,0,1)
        glVertex3f(0,0,0)
        glVertex3f(0,0,length)
        glEnd()

        glPopAttrib()

    def display(self):
        H_IDX = 0 # calling shape on frame returns tuple, 0th index represents height
        W_IDX = 1 # calling shape on frame returns tuple, 1st index represents width
        # clear the window
        glClear(GL_COLOR_BUFFER_BIT)

        # show the current camera frame
        # based on the way opencv stores data, you need to flip it before displaying it
        self.flipped_frame = cv2.flip(self.frame, 0)
        glDrawPixels(self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX], GL_BGR, GL_UNSIGNED_BYTE, self.flipped_frame)

        ########################################################################################################################
        # here, set up new parameters to render a scene viewed from the camera

        # set viewport
        glViewport(0, 0, self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX])

        # set projection matrix using intrinsic camera params
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()

        # gluPerspective is arbitrarily set, you will have to determine these values based on the intrinsic camera parameters
        fovy = 2*math.atan(self.flipped_frame.shape[H_IDX]/(2*fy))
        #print("fovy: "+str(fovy))
        gluPerspective(fovy*180/math.pi, self.flipped_frame.shape[W_IDX] * 1.0 / self.flipped_frame.shape[H_IDX], 1, 20)

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0)

        ########################################################################################################################
        # drawing routine

        #build view matrix
        gray = cv2.cvtColor(self.flipped_frame, cv2.COLOR_BGR2GRAY)
        ret, corners = cv2.findChessboardCorners(gray, (board_width, board_height), None)
        if ret == True:
            corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
            ret, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)

            #rodriges rotation
            rodrigues_matrix, _ = cv2.Rodrigues(rvecs)

            #create Rt Homogeneous Transformation matrix
            viewMatrix = np.hstack((rodrigues_matrix, tvecs))
            viewMatrix = np.vstack((viewMatrix, np.array((0, 0, 0, 1))))
            print("viewMatrix1: "+ str(viewMatrix))

            #Multiply the view matrix by the transfer matrix between OpenCV and OpenGL
            cvToGl = np.array([[1.0, 1.0, 1.0, 1.0],
            [1.0, -1.0, 1.0, 1.0], #invert y-axis
            [1.0, 1.0, -1.0, 1.0], #invert z-axis
            [1.0, 1.0, 1.0, 1.0]])
            viewMatrix = cvToGl * viewMatrix;
            print("viewMatrix2: "+ str(viewMatrix))

            #Because OpenCV's matrixes are stored by row you have to transpose the matrix in order that OpenGL can read it by colum
            viewMatrix = viewMatrix.T
            print("viewMatrix3: "+ str(viewMatrix))
            #glMatrixMode(GL_MODELVIEW);
            #glTranslatef(0, 0, 0)
            glLoadMatrixf(viewMatrix.flatten());

            glutSolidTeapot(0.5)
            self.draw_axes(1.0)

        #re-calculate camera extrinsic per frame
        '''gray = cv2.cvtColor(self.flipped_frame, cv2.COLOR_BGR2GRAY)
        ret, corners = cv2.findChessboardCorners(gray, (board_width, board_height), None)
        if ret == True:
            corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
            ret, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)

            #rodriges rotation
            rodrigues_matrix, _ = cv2.Rodrigues(rvecs)
            print("rodrigues_matrix: "+str(rodrigues_matrix))

            #tranlation glMatrixMode
            print("tvecs: "+str(tvecs))
            print("rvecs: "+str(rvecs))

            #create Rt Homogeneous Transformation matrix
            Rt = np.hstack((rodrigues_matrix, tvecs))
            print("Rt: "+str(Rt))
            Rt = np.vstack((Rt, np.array((0, 0, 0, 1))))
            print("Rt: "+str(Rt))

            # rotate teapot 90 deg around x-axis so that z-axis is up
            Rx = np.array([[1,0,0],[0,0,-1],[0,1,0]]) #equivalent to lookUp

            # set rotation to best approximation
            R = Rt[:,:3]
            U,S,V = np.linalg.svd(R)
            print("U: "+str(U[:,:3]))
            print("V: "+str(V))
            R = np.dot(U[:,:3], V)
            R[0,:] = -R[0,:] # change sign of x-axis

            # set translation
            t = Rt[:,3]

            # setup 4*4 model view matrix
            M = np.eye(4)
            print("R: "+str(R[:3,:3]))
            print("Rx: "+str(Rx))
            M[:3,:3] = np.dot(R[:3,:3],Rx)
            print("t: "+str(t[:3]))
            M[:3,3] = t[:3]

            # transpose and flatten to get column order
            M = M.T
            m = M.flatten()

            # replace model view with the new matrix
            glLoadMatrixf(m)

            glutSolidTeapot(0.5)
            self.draw_axes(1.0)'''

        '''# now that the camera params have been set, draw your 3D shapes
        # first, save the current matrix
        glPushMatrix()
        # move to the position where you want the 3D object to go
        glTranslatef(0, 0, 0) # this is an arbitrary position for demonstration
        # you will need to adjust your transformations to match the positions where
        # you want to draw your objects(i.e. chessboard center, chessboard corners)
        glutSolidTeapot(0.5)
        # glutSolidSphere(.3, 100, 100);
        self.draw_axes(1.0)
        glPopMatrix()'''

        # show the rendering on the screen
        glutSwapBuffers()

        # post the next redisplay
        glutPostRedisplay()

    def reshape(self, w, h):
        '''set openGL viewport (drawable area)'''
        glViewport(0, 0, w, h)

    def mouse(self, button, state, x, y):
        if button == GLUT_LEFT_BUTTON and state == GLUT_UP:
            pass

    def keyboard(self, key, x, y):
        if key == b'q':
            # quit when 'q' is pressed
            sys.exit(0)
        else:
            pass

    def idle(self):
        '''grabs a frame from the camera'''
        ret, frame = self.cap.read()
        if ret == False:
            print('no frames to grab, exiting')
            sys.exit(0)
        self.frame = frame

def main(argv):
    if len(argv) == 1:
        source = 0
    else:
        source = argv[1]

    ogl_cv = OpenGL_CV(source)
    ogl_cv.idle()

    # initialize GLUT
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE)
    glutInitWindowPosition(20, 20)
    glutInitWindowSize(ogl_cv.width, ogl_cv.height)
    glutCreateWindow("OpenGL / OpenCV Example")

    # set up GUI callback functions
    glutDisplayFunc(ogl_cv.display)
    glutReshapeFunc(ogl_cv.reshape)
    glutMouseFunc(ogl_cv.mouse)
    glutKeyboardFunc(ogl_cv.keyboard)
    glutIdleFunc(ogl_cv.idle)

    # start GUI loop
    #while(1):
    glutMainLoop()

if __name__ == '__main__':
    main(sys.argv)
