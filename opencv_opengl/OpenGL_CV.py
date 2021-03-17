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

board_width=8
board_height=6
# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((board_height*board_width,3), np.float32)
objp[:,:2] = np.mgrid[0:board_width,0:board_height].T.reshape(-1,2)
# termination criteria
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

dist = np.array([[ 0.05267650832192627, -0.4956190413924691, 0.0052524038601472495, 0.003143951311043761, 1.7247203077112738]])
mtx = np.array([[654.0386532237726, 0., 327.34270215404126],
                [0., 654.9263071068098, 251.69899149056258],
                [0., 0., 1.]])

# my calibration matrix
#dist = np.array([[ 0.09016813588756717, -0.196871593424749, -0.005570823681104732, -0.0011444615644275139, -0.11395723224378385]])
#mtx = np.array([[910.8843804968606, 0., 620.0830549034282],
#                [0., 914.9719257777986, 373.9797758912618],
#                [0., 0., 1.]])

H_IDX = 0 # calling shape on frame returns tuple, 0th index represents height
W_IDX = 1 # calling shape on frame returns tuple, 1st index represents width

axis = np.float32([[3,0,0], [0,3,0], [0,0,-3]]).reshape(-1,3)

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

    def drawAxis(self, img, corners, imgpts):
        corner = tuple(corners[0].ravel())
        img = cv2.line(img, corner, tuple(imgpts[0].ravel()), (255,0,0), 5)
        img = cv2.line(img, corner, tuple(imgpts[1].ravel()), (0,255,0), 5)
        img = cv2.line(img, corner, tuple(imgpts[2].ravel()), (0,0,255), 5)
        return img

    def drawTeaPot(self):
        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        #gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0)

        ########################################################################################################################
        # drawing routine

        #undistortion
        img = self.flipped_frame
        h,  w = img.shape[:2]
        newcameramtx, roi=cv2.getOptimalNewCameraMatrix(mtx,dist,(w,h),1,(w,h))

        # undistort
        dst = cv2.undistort(img, mtx, dist, None, newcameramtx)

        # crop the image
        x,y,w,h = roi
        dst = dst[y:y+h, x:x+w]
        #cv2.imwrite('calibresult.png',dst)
        self.flipped_frame = dst

        gray = cv2.cvtColor(self.flipped_frame, cv2.COLOR_BGR2GRAY)
        ret, corners = cv2.findChessboardCorners(gray, (board_width, board_height), None)
        if ret == True:
            #corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
            corners2 = corners
            ret, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)

            rmtx = cv2.Rodrigues(rvecs)[0]

            view_matrix = np.array([[rmtx[0][0],rmtx[0][1],rmtx[0][2],tvecs[0]+board_width/2],
                                [rmtx[1][0],rmtx[1][1],rmtx[1][2],tvecs[1]+board_height/2+0.5],
                                [rmtx[2][0],rmtx[2][1],rmtx[2][2],tvecs[2]],
                                [0.0       ,0.0       ,0.0       ,1.0    ]])

            inverse_matrix = np.array([[ 1.0, 1.0, 1.0, 1.0],
                                   [1.0,1.0,1.0,1.0],
                                   [-1.0,-1.0,-1.0,-1.0],
                                   [ 1.0, 1.0, 1.0, 1.0]])

            view_matrix = view_matrix * inverse_matrix
            view_matrix = np.transpose(view_matrix)

            glPushMatrix()
            glLoadMatrixd(view_matrix)
            glutSolidTeapot(1.0)
            self.draw_axes(1.0)
            glPopMatrix()

    def drawSpheres(self):
        #glTranslatef(1.0, 1.0, 1.0)

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        #gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0)

        gray = cv2.cvtColor(self.flipped_frame, cv2.COLOR_BGR2GRAY)
        ret, corners = cv2.findChessboardCorners(gray, (board_width, board_height), None)

        #undistortion
        img = self.flipped_frame
        h,  w = img.shape[:2]
        newcameramtx, roi=cv2.getOptimalNewCameraMatrix(mtx,dist,(w,h),1,(w,h))

        # undistort
        dst = cv2.undistort(img, mtx, dist, None, newcameramtx)

        # crop the image
        x,y,w,h = roi
        #dst = dst[y:y+h, x:x+w]
        #cv2.imwrite('calibresult.png',dst)
        self.flipped_frame = dst

        self.flipped_frame = cv2.drawChessboardCorners(self.flipped_frame, (board_width, board_height), corners, True)
        glDrawPixels(self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX], GL_BGR, GL_UNSIGNED_BYTE, self.flipped_frame)

        if ret == True:
            #corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
            corners2 = corners
            print("corners2: "+str((corners2)))

            ret, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)
            print("rvecs: "+str(rvecs))
            print("tvecs: "+str(tvecs))
            rmtx = cv2.Rodrigues(rvecs)[0]

            # project 3D points to image plane
            #imgpts, jac = cv2.projectPoints(axis, rvecs, tvecs, mtx, dist)
            #self.flipped_frame = self.drawAxis(self.flipped_frame, corners2, imgpts)
            #glDrawPixels(self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX], GL_BGR, GL_UNSIGNED_BYTE, self.flipped_frame)

            view_matrix = np.array([[rmtx[0][0],rmtx[0][1],rmtx[0][2],tvecs[0]+0.5],
                                [rmtx[1][0],rmtx[1][1],rmtx[1][2],tvecs[1]+0.5],
                                [rmtx[2][0],rmtx[2][1],rmtx[2][2],tvecs[2]],
                                [0.0       ,0.0       ,0.0       ,1.0    ]])

            inverse_matrix = np.array([[ 1.0, 1.0, 1.0, 1.0],
                                   [1.0,1.0,1.0,1.0],
                                   [-1.0,-1.0,-1.0,-1.0],
                                   [ 1.0, 1.0, 1.0, 1.0]])

            view_matrix = view_matrix * inverse_matrix
            view_matrix = np.transpose(view_matrix)

            glPushMatrix()
            glLoadMatrixd(view_matrix)
            self.draw_axes(1.0)
            for i in range(8):
                for j in range(6):
                    glutSolidSphere(0.2, 20, 20)
                    #print("corners[i]: "+str(corners2[i]))
                    glTranslatef(0, 1, 0)
                glTranslatef(1, 0, 0)
                glTranslatef(0, -6, 0)
            glPopMatrix()

    def display(self):
        # clear the window
        #glClear(GL_COLOR_BUFFER_BIT)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

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
        fovy = 2*math.atan(self.flipped_frame.shape[H_IDX]/(2*mtx[1,1]))*180/math.pi
        #fovx, fovy, _ = cv2.calibrationMatrixValues(mtx, (self.flipped_frame.shape[H_IDX], self.flipped_frame.shape[W_IDX]), 0)
        #print("fovy: "+str(fovy_old)+"::"+str(fovy))
        gluPerspective(fovy, self.flipped_frame.shape[W_IDX] * 1.0 / self.flipped_frame.shape[H_IDX], 0.01, 200)

        #self.drawTeaPot()
        self.drawSpheres()

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
        elif key == b's':
            cv2.imwrite('screenshot'+'.png', self.frame)
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
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
    glutInitWindowPosition(20, 20)
    glutInitWindowSize(ogl_cv.width, ogl_cv.height)
    glutCreateWindow("OpenGL / OpenCV Example")

    #shading in openGL
    glShadeModel(GL_SMOOTH)
    #glEnable(GL_CULL_FACE)
    #glEnable(GL_DEPTH_TEST)
    glClearDepth(1.0)
    glEnable(GL_LIGHTING)
    lightZeroPosition = [-20.,2.,-2.,1.]
    lightZeroColor = [1.8,1.0,0.8,1.0] #green tinged
    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor)
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1)
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05)
    glEnable(GL_LIGHT0)

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
