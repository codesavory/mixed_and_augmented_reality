import numpy as np
import cv2
import sys
import pickle

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

class face_augmentation:

    def __init__(self, source):
        #define a video capture object
        self.vcap = cv2.VideoCapture(source)
        if self.vcap.isOpened() == False:
            print("Error opening video stream...exiting")
            sys.exit(0)
        self.frame = None
        self.width = int(self.vcap.get(3))
        self.height = int(self.vcap.get(4))
        #Haar Cascade Filter for face detection
        self.face_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
        #Camera Calibration parameters
        if len(sys.argv) == 2:  #python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
            with open(sys.argv[1], "rb") as input:
                self.calCam = pickle.load(input)
                #print(self.calCam)
        elif len(sys.argv) == 3:
            with open(sys.argv[2], "rb") as input:
                self.calCam = pickle.load(input)

        #height of my face
        self.faceRealHeight = 24.0 #calculated manually
        self.faceRealX = None
        self.faceRealY = None
        self.faceRealZ = None

        #height of face seen by camera
        self.faceImageHeight = None #assume square face
        self.faceImageX = None
        self.faceImageY = None
        self.faceImageU = None
        self.faceImageV = None

        self.M = None
        self.ret = None
        self.rot = 0

        self.renderList = ["Cone", "Torus", "Teapot"]
        self.renderObject = self.renderList[2]

        self.bgImg = None
        self.diff = None

    def keyboard(self, key, x, y):
        if key == b'q':
            # quit when 'q' is pressed
            sys.exit(0)

        if key == b's':
            self.bgImg = self.frame
            #cv2.imshow("check",self.bgImg)

        if key == b'g':
            cv2.imshow("diff",self.diff)

    def get_persepective_matrix(self):
        # creating perspective matrix: https://blog.noctua-software.com/opencv-opengl-projection-matrix.html
        P = np.zeros(shape=(4, 4), dtype=np.float32)

        fx = self.calCam[1][0][0]
        fy = self.calCam[1][1][1]

        cx = self.calCam[1][0][-1]
        cy = self.calCam[1][1][-1]

        near = 0.1
        far = 500.0

        P[0][0] = 2*fx / self.width
        P[1][1] = 2*fy / self.height
        P[0][2] = 1 - (2*cx / self.width)
        P[1][2] = (2*cy / self.height - 1)
        P[2][2] = -(far + near) / (far - near)
        P[3][2] = -1.
        P[2][3] = -(2*far*near) / (far - near)

        p = P.T
        return p

    def draw_cube(self):
        glBegin(GL_QUADS)  #Begin drawing the color cube with 6 quads

        # Front face  (z = 0.0f)
        glColor3f(1.0, 0.0, 0.0)     # Red
        glVertex3f(0.0, 0.0, 0.0)
        glVertex3f(1.0, 0.0, 0.0)
        glVertex3f(1.0, 1.0, 0.0)
        glVertex3f(0.0, 1.0, 0.0)

        # Back face (z = -1.0f)
        glColor3f(1.0, 1.0, 0.0)     # Yellow
        #glTranslatef(0.0, -0.2, 0.0)
        offset = -0.2
        glVertex3f(0.0+offset, 0.0+offset, -1.0)
        glVertex3f(1.0+offset, 0.0+offset, -1.0)
        glVertex3f(1.0+offset, 1.0+offset, -1.0)
        glVertex3f(0.0+offset, 1.0+offset, -1.0)
        #glTranslatef(0.0, 0.2, 0.0)

        glEnd()  # End of drawing color-cube

    def draw_sphere(self, w, h):
        for x in range(w):
            for y in range(h):
                #print("Inside Draw Sphere")
                glPushMatrix()
                glTranslatef(x, y, 0)
                glutSolidSphere(0.2, 100, 100)
                glPopMatrix()

    def draw_teapot(self, w, h):
        print("Inside Draw Teapot")
        glEnable(GL_DEPTH_TEST)
        glPushMatrix()
        #glTranslatef(w/2.0, h/2.0, 0)
        glTranslatef(0, 0, 0)

        #draw occlueded cube/sphere
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE)
        #glTranslatef(0.5, 0.5, -0.5)
        #glutSolidSphere(0.5, 100, 100)
        self.draw_cube()
        #glTranslatef(-0.5, -0.5, 0.5)
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE)

        glRotatef(90, 0, 0, 1)
        glTranslatef(0.0, -0.4, 0.0)
        #glRotatef(90, 1, 0, 0)
        glTranslatef(0.5, 0.5, -0.5)
        glRotatef(self.rot, 0, 1, 0)
        glTranslatef(-0.5, -0.5, 0.5)
        glutSolidTeapot(0.3)
        glPopMatrix()
        glDisable(GL_DEPTH_TEST)

    def display(self):
        H_IDX = 0  # calling shape on frame returns tuple, 0th index represents height
        W_IDX = 1  # calling shape on frame returns tuple, 1st index represents width
        # clear the window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        # show the current camera frame
        # based on the way opencv stores data, you need to flip it before displaying it
        self.flipped_frame = cv2.flip(self.frame, 0) #0 - flip about x - axis(vertical), 1 - flip horizontal and -1 - flip both axis
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
        glViewport(0, 0, self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX])

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0)
        if self.ret == True:
            glPushMatrix()
            #glTranslatef(0, 0, 0)
            glLoadMatrixd(self.M)
            ########################################################################################################################
            # drawing routine
            # now that the camera params have been set, draw your 3D shapes
            # first, save the current matrix
            #print("Drawing Sphere")
            self.draw_teapot(0, 0)
            glPopMatrix()

        # show the rendering on the screen
        glutSwapBuffers()

        # post the next redisplay
        glutPostRedisplay()

    #gesture tracking
    def checkGesture(self):
        first_gray = cv2.cvtColor(self.bgImg, cv2.COLOR_BGR2GRAY)
        first_gray = cv2.GaussianBlur(first_gray, (5, 5), 0)

        gray_frame = cv2.cvtColor(self.frame, cv2.COLOR_BGR2GRAY)
        gray_frame = cv2.GaussianBlur(gray_frame, (5, 5), 0)

        difference = cv2.absdiff(first_gray, gray_frame)
        _, difference = cv2.threshold(difference, 25, 255, cv2.THRESH_BINARY)
        self.diff = difference

        #cv2.imshow("First frame", self.bgImg)
        #cv2.imshow("Frame", self.frame)
        #cv2.imshow("difference", difference)

    def image_show(self):
        #while(True):
        ret, frame = self.vcap.read()
        if ret == False:
            print('no frames to grab, exiting')
            sys.exit(0)
        self.frame = frame
        if self.bgImg is None: #set the first frame in bgImg
            self.bgImg = self.frame
        gray = cv2.cvtColor(self.frame, cv2.COLOR_BGR2GRAY)
        faces = self.face_cascade.detectMultiScale(gray, 1.1, 5)

        #add gesture overlay
        self.frame = cv2.rectangle(self.frame, (self.width, 0), (self.width-50, 50), (0,0,255), -1) #red overlay
        self.frame = cv2.rectangle(self.frame, (0, 0), (50, 50), (0,255,0), -1)                     #green overlay
        self.checkGesture()

        for (x,y,w,h) in faces:
            self.ret = True #detected a face
            self.frame = cv2.rectangle(self.frame,(x,y),(x+w,y+h),(0, 255, 255),1)
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = self.frame[y:y+h, x:x+w]
            self.faceImageHeight = h
            self.faceImageU = w/2
            self.faceImageV = h/2
            self.faceImageX = x
            self.faceImageY = y

            #basic projection equation to find distance of real face
            fx = self.calCam[1][0][0]
            fy = self.calCam[1][1][1]
            self.faceRealZ = (fy*self.faceRealHeight)/(self.faceImageHeight)
            #print("faceRealZ: "+str(self.faceRealZ))

            #calculate rectangle detector in 3D
            self.faceRealX = ((self.faceImageX - self.faceImageU)*self.faceRealZ)/fx
            self.faceRealY = ((self.faceImageY - self.faceImageV)*self.faceRealZ)/fy

            print("X,Y,Z: "+str(self.faceRealX)+", "+str(self.faceRealY)+", "+str(self.faceRealZ))

            # storing object points and image points for frame
            objp = np.zeros((4, 3), dtype=np.float64)
            '''objp[0] = np.array([self.faceRealX, self.faceRealY, self.faceRealZ])
            objp[1] = np.array([self.faceRealX+self.faceRealHeight, self.faceRealY, self.faceRealZ])
            objp[2] = np.array([self.faceRealX+self.faceRealHeight, self.faceRealY+self.faceRealHeight, self.faceRealZ])
            objp[3] = np.array([self.faceRealX, self.faceRealY+self.faceRealHeight, self.faceRealZ])'''
            objp[0] = np.array([0,0,0])
            objp[1] = np.array([0,1,0])
            objp[2] = np.array([1,1,0])
            objp[3] = np.array([1,0,0])
            #objp[:, :2] = np.mgrid[0:8, 0:6].T.reshape(-1, 2)
            #self.objp = objp

            # finding corners of the rect, image points
            corners = np.zeros((4, 2), dtype=np.float64)
            corners[0] = np.array([x, y])
            corners[1] = np.array([x+w, y])
            corners[2] = np.array([x+w, y+h])
            corners[3] = np.array([x, y+h])
            print("corners: "+str(corners))
            #self.corners = corners
            #self.ret = ret

            #self.objpoints3D.append(objp)
            #self.imgpoints2D.append(corners)
            # getting model view matrix
            # https://stackoverflow.com/questions/44375149/opencv-to-opengl-coordinate-system-transform
            ret_, rvec, tvec = cv2.solvePnP(
                objp, corners, self.calCam[1], self.calCam[2])

            print(ret_, rvec, tvec)
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
            #M = [[0, 0, 0, tvec[0][0]], [0, 0, 0, tvec[1][0]], [0, 0, 0, tvec[2][0]], [0, 0, 0, 1]]
            print("M:"+str(M))
            self.M = np.array(M).T

            self.rot+=5.0

            '''#calculate view matrix
            view_matrix = np.array([[0.0,  0.0,  0.0,  self.faceRealX],
                                    [0.0,  0.0,  0.0,  self.faceRealY],
                                    [0.0,  0.0,  0.0,  self.faceRealZ],
                                    [0.0,  0.0,  0.0,  1.0]])

            inverse_matrix = np.array([ [1.0,   1.0,    1.0,    1.0],
                                        [1.0,   1.0,    1.0,    1.0],
                                        [-1.0, -1.0,   -1.0,   -1.0],
                                        [1.0,   1.0,    1.0,    1.0]])

            view_matrix = view_matrix * inverse_matrix
            view_matrix = np.transpose(view_matrix)
            self.M = view_matrix'''

        #cv2.imshow("Interactive Face Augmentation", self.frame)
        #self.vcap.release()
        #cv2.waitKey(0)
        #cv2.destroyAllWindows()

def main(argv):
    if len(argv) == 2: #python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
        source = 0
    elif len(argv) == 3:
        source = argv[1]
    else:
        print("Add camera calib file to argv")
        sys.exit(0)

    face_augment = face_augmentation(source)
    face_augment.image_show()

    # initialize GLUT
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
    # glutInitWindowPosition(20, 20)
    glutInitWindowSize(face_augment.width, face_augment.height)
    glutCreateWindow("OpenGL/OpenCV Interactive Face Augmentation")

    #shading in OpenGL
    glEnable(GL_LIGHTING)
    lightZeroPosition = [-20.,2.,-2.,1.]
    #lightZeroColor = [1.8,1.0,0.8,1.0]
    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition)
    #glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor)
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1)
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05)
    glColor(1.0, 0.0, 0.0)
    glEnable(GL_LIGHT0)

    #depth in opengl
    #glDepthFunc(GL_LESS)
    glEnable(GL_DEPTH_TEST)
    #glDepthMask(GL_FALSE)

    # set up GUI callback functions
    glutDisplayFunc(face_augment.display)
    glutKeyboardFunc(face_augment.keyboard)
    glutIdleFunc(face_augment.image_show)

    # start GUI loop
    glutMainLoop()

if __name__ == '__main__':
    main(sys.argv)
