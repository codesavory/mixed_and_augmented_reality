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
        gluPerspective(60, self.flipped_frame.shape[W_IDX] * 1.0 / self.flipped_frame.shape[H_IDX], 1, 20)

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0)

        ########################################################################################################################
        # drawing routine

        # now that the camera params have been set, draw your 3D shapes
        # first, save the current matrix
        glPushMatrix()
        # move to the position where you want the 3D object to go
        glTranslatef(0, 0, 0) # this is an arbitrary position for demonstration
        # you will need to adjust your transformations to match the positions where
        # you want to draw your objects(i.e. chessboard center, chessboard corners)
        glutSolidTeapot(0.5)
        # glutSolidSphere(.3, 100, 100);
        self.draw_axes(1.0)
        glPopMatrix()

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
    glutMainLoop()

if __name__ == '__main__':
    main(sys.argv)
