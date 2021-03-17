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

    def keyboard(self, key, x, y):
        if key == b'q':
            # quit when 'q' is pressed
            sys.exit(0)
        '''if key == b' ':
            if self.keyPress == False:
                self.keyPress = True
            elif self.keyPress == True:
                self.keyPress = False
        else:
            pass'''

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

    def draw_sphere(self, w, h):
        for x in range(w):
            for y in range(h):
                print("Inside Draw Sphere")
                glPushMatrix()
                glTranslatef(x, y, 0)
                glutSolidSphere(0.2, 100, 100)
                glPopMatrix()

    def draw_teapot(self, w, h):
        #print("Inside Draw Teapot")
        glPushMatrix()
        #glTranslatef(w/2.0, h/2.0, 0)
        #glRotatef(270, 1, 0, 0)
        glutSolidTeapot(15.0)
        glPopMatrix()

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
            #print("Drawing Sphere")
            self.draw_teapot(0, 0)
            glPopMatrix()

        # show the rendering on the screen
        glutSwapBuffers()

        # post the next redisplay
        glutPostRedisplay()

    def image_show(self):
        #while(True):
        ret, frame = self.vcap.read()
        if ret == False:
            print('no frames to grab, exiting')
            sys.exit(0)
        self.frame = frame
        gray = cv2.cvtColor(self.frame, cv2.COLOR_BGR2GRAY)
        faces = self.face_cascade.detectMultiScale(gray, 1.1, 5)

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
            self.faceRealZ = (fy*self.faceRealHeight)/(self.faceImageHeight-self.faceImageV)
            #print("faceRealZ: "+str(self.faceRealZ))

            #calculate rectangle detector in 3D
            self.faceRealX = ((self.faceImageX - self.faceImageU)*self.faceRealZ)/fx
            self.faceRealY = ((self.faceImageY - self.faceImageV)*self.faceRealZ)/fy

            print("X,Y,Z: "+str(self.faceRealX)+", "+str(self.faceRealY)+", "+str(self.faceRealZ))
            #calculate view matrix
            view_matrix = np.array([[0.0,  0.0,  0.0,  self.faceRealX],
                                    [0.0,  0.0,  0.0,  self.faceRealY],
                                    [0.0,  0.0,  0.0,  self.faceRealZ],
                                    [0.0,  0.0,  0.0,  1.0]])

            inverse_matrix = np.array([ [1.0,   1.0,    1.0,    1.0],
                                        [1.0,   1.0,    1.0,    1.0],
                                        [1.0,   1.0,    -1.0,    1.0],
                                        [1.0,   1.0,    1.0,    1.0]])

            view_matrix = view_matrix * inverse_matrix
            view_matrix = np.transpose(view_matrix)
            self.M = view_matrix
        #cv2.imshow("Interactive Face Augmentation", self.frame)
        #self.vcap.release()
        #cv2.waitKey(0)
        #cv2.destroyAllWindows()

def main(argv):
    if len(argv) == 2: #python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
        source = 0
    elif len(argv) == 3:
        source = argv[1]

    face_augment = face_augmentation(source)
    face_augment.image_show()

    # initialize GLUT
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE)
    # glutInitWindowPosition(20, 20)
    glutInitWindowSize(face_augment.width, face_augment.height)
    glutCreateWindow("OpenGL/OpenCV Interactive Face Augmentation")

    # set up GUI callback functions
    glutDisplayFunc(face_augment.display)
    glutKeyboardFunc(face_augment.keyboard)
    glutIdleFunc(face_augment.image_show)

    # start GUI loop
    glutMainLoop()

if __name__ == '__main__':
    main(sys.argv)
