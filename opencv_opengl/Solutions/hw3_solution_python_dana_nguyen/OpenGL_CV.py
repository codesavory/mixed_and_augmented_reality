from cv2 import cv2
import numpy as np
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import sys
import math
import pickle
import time


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
        self.exCalCam = None
        self.M = None
        self.corners = None
        self.objp = None
        self.projPoints = None
        self.keyPress = False
        self.objpoints3D = []  # 3d point in real world space
        self.imgpoints2D = []  # 2d points in image plane.
        self.tvec = None
        self.rvec = None
        self.rotM = None
        self.ret = False
        #self.calCam = None
        if len(sys.argv) == 2:
            with open(sys.argv[1], "rb") as input:
                self.calCam = pickle.load(input)
                print("calCam:"+str(calCam))
        #elif len(sys.argv) == 3:
        else:
            print("argv[2]:"+str(sys.argv[2]))
            with open(sys.argv[2], "rb") as input:
                self.calCam = pickle.load(input)
                print("calCam:"+str(self.calCam))
        pass

    def draw_axes(self, length):
        '''a useful function to display your coordinate system'''
        glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
        glDisable(GL_LIGHTING)

        glBegin(GL_LINES)
        glColor3f(1, 0, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(length, 0, 0)

        glColor3f(0, 1, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, length, 0)

        glColor3f(0, 0, 1)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, length)
        glEnd()

        glPopAttrib()

    def display(self):
        H_IDX = 0  # calling shape on frame returns tuple, 0th index represents height
        W_IDX = 1  # calling shape on frame returns tuple, 1st index represents width
        # clear the window
        glClear(GL_COLOR_BUFFER_BIT)

        # show the current camera frame
        # based on the way opencv stores data, you need to flip it before displaying it
        self.flipped_frame = cv2.flip(self.frame, 0)
        glDrawPixels(self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX],
                     GL_BGR, GL_UNSIGNED_BYTE, self.flipped_frame)

        ########################################################################################################################
        # here, set up new parameters to render a scene viewed from the camera

        # set projection matrix using intrinsic camera params
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()

        # set perspective matrix
        glLoadMatrixf(self.get_persepective_matrix())

        # set viewport
        glViewport(
            0, 0, self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX])

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        if self.ret == True:
            glLoadMatrixf(self.M)

            ########################################################################################################################
            # drawing routine
            # now that the camera params have been set, draw your 3D shapes
            # first, save the current matrix
            glPushMatrix()
            if self.keyPress:
                self.draw_sphere(8, 6)
            else:
                self.draw_teapot(8, 6)
            self.draw_axes(1.0)
            glPopMatrix()

        # show the rendering on the screen
        glutSwapBuffers()

        # post the next redisplay
        glutPostRedisplay()

    def get_persepective_matrix(self):
        # creating perspective matrix: https://blog.noctua-software.com/opencv-opengl-projection-matrix.html
        P = np.zeros(shape=(4, 4), dtype=np.float32)

        fx = self.calCam[1][0][0]
        fy = self.calCam[1][1][1]

        cx = self.calCam[1][0][-1]
        cy = self.calCam[1][1][-1]

        near = 0.1
        far = 100.0

        P[0][0] = 2*fx / self.width
        P[1][1] = 2*fy / self.height
        P[0][2] = 1 - (2*cx / self.width)
        P[1][2] = (2*cy / self.height - 1)
        P[2][2] = -(far + near) / (far - near)
        P[3][2] = -1.
        P[2][3] = -(2*far*near) / (far - near)

        p = P.T
        return p

    def draw_sphere(self, w, h):
        for x in range(w):
            for y in range(h):
                glPushMatrix()
                glTranslatef(x, y, 0)
                glutSolidSphere(0.2, 100, 100)
                glPopMatrix()

    def draw_teapot(self, w, h):
        glPushMatrix()
        glTranslatef(w/2.0, h/2.0, 0)
        glRotatef(270, 1, 0, 0)
        glutSolidTeapot(1.5)
        glPopMatrix()

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
        if key == b' ':
            if self.keyPress == False:
                self.keyPress = True
            elif self.keyPress == True:
                self.keyPress = False
        else:
            pass

    def draw_debug_circles(self):
        # used to debug on checkerboard
        imgpoints2D2, _ = cv2.projectPoints(
            self.objp, self.rvec, self.tvec, self.calCam[1], self.calCam[2])

        for i in imgpoints2D2:
            coordinate = (i[0][0], i[0][1])
            cv2.circle(self.frame, coordinate, 5, color=(255, 0, 0))

    def idle(self):
        '''grabs a frame from the camera'''
        ret_frame, frame = self.cap.read()

        if ret_frame == False:
            print('no frames to grab, exiting')
            sys.exit(0)

        self.frame = frame

        # used to slow down the video
        # time.sleep(1)

        # storing object points and image points for frame
        objp = np.zeros((6*8, 3), np.float32)
        objp[:, :2] = np.mgrid[0:8, 0:6].T.reshape(-1, 2)
        self.objp = objp

        # finding corners of the frame
        ret, corners = cv2.findChessboardCorners(frame, (8, 6), None)
        self.corners = corners
        self.ret = ret

        if ret == True:
            self.objpoints3D.append(objp)
            self.imgpoints2D.append(corners)
            # getting model view matrix
            # https://stackoverflow.com/questions/44375149/opencv-to-opengl-coordinate-system-transform
            ret_, rvec, tvec = cv2.solvePnP(
                objp, corners, self.calCam[1], self.calCam[2])

            # print(ret_, rvec, tvec)
            self.tvec = tvec
            self.rvec = rvec
            rotM = cv2.Rodrigues(rvec)[0]
            M = [[], [], [], [0, 0, 0, 1]]
            self.rotM = rotM
            for idx, val in enumerate(rotM):
                # invert y and z axis
                if idx == 1 or idx == 2:
                    M[idx] = np.multiply(
                        np.array(list(rotM[idx]) + [tvec[idx][0]]), -1)
                    # print(np.dot(rotM[idx], Rx[idx]))
                else:
                    M[idx] = np.array(
                        list(rotM[idx])+[tvec[idx][0]])
            self.M = np.array(M).T


def main(argv):
    if len(argv) == 2:
        source = 0
    else:
        source = argv[1]

    print("source:"+str(source))
    ogl_cv = OpenGL_CV(source)
    ogl_cv.idle()

    # initialize GLUT
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE)
    # glutInitWindowPosition(20, 20)
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
