import numpy as np
import cv2
import sys
import pickle

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

import mediapipe_apis as mpa
import json
import open3d as o3d

class faceRelighting:
    def __init__(self, source):
        #define a video capture object
        self.vcap = cv2.VideoCapture(source)
        if self.vcap.isOpened() == False:
            print("Error opening video stream...exiting")
            sys.exit(0)
        self.frame = None
        self.width = int(self.vcap.get(3))
        self.height = int(self.vcap.get(4))

        #Camera Calibration parameters
        if len(sys.argv) == 2:  #python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
            with open(sys.argv[1], "rb") as input:
                self.calCam = pickle.load(input)
                #print(self.calCam)
        elif len(sys.argv) == 3:
            with open(sys.argv[2], "rb") as input:
                self.calCam = pickle.load(input)

        self.faceMeshPoints = None
        self.ret = False

        self.mesh_vertices = None
        self.mesh_surfaces = None
        self.mesh_normals = None

    def keyboard(self, key, x, y):
        if key == b'q':
            # quit when 'q' is pressed
            sys.exit(0)

    def get_persepective_matrix(self):
        # creating perspective matrix: https://blog.noctua-software.com/opencv-opengl-projection-matrix.html
        P = np.zeros(shape=(4, 4), dtype=np.float32)

        fx = self.calCam[1][0][0]
        fy = self.calCam[1][1][1]

        cx = self.calCam[1][0][-1]
        cy = self.calCam[1][1][-1]

        near = 0.1
        far = 5000.0

        P[0][0] = 2*fx / self.width
        P[1][1] = 2*fy / self.height
        P[0][2] = 1 - (2*cx / self.width)
        P[1][2] = (2*cy / self.height - 1)
        P[2][2] = -(far + near) / (far - near)
        P[3][2] = -1.
        P[2][3] = -(2*far*near) / (far - near)

        p = P.T
        return p

    def drawFaceMesh(self, meshPoints):
        glEnable(GL_DEPTH_TEST)
        for point in meshPoints:
            glPushMatrix()
            #print(point)
            #glScale(5, -5, -5)
            glTranslatef(point[0], point[1], point[2])
            glutSolidSphere(0.05, 100, 100)
            glPopMatrix()
        glDisable(GL_DEPTH_TEST)

    def drawFaceMeshQuad(self, meshPoints):
        glPushMatrix()
        #glScale(5, -5, -5)
        glBegin(GL_TRIANGLES)
        for i in range(0, len(meshPoints), 3):
            #glTranslatef(meshPoints[i][0], meshPoints[i][1], meshPoints[i][2])
            v1 = np.array(meshPoints[i])
            v2 = np.array(meshPoints[i+1])
            v3 = np.array(meshPoints[i+2])
            #v4 = meshPoints[i+3]
            glVertex3fv(v1)
            glVertex3fv(v2)
            glVertex3fv(v3)
            #glVertex3fv(v4)
            normal = np.cross(v1-v2, v3-v2)
            normal = normal/np.linalg.norm(normal)
            #print(normal)
            glNormal3fv(normal)
        glEnd()
        glPopMatrix()

    def draw_teapot(self, w, h):
        glPushMatrix()
        glTranslatef(w/2.0, h/2.0, 0)
        glRotatef(270, 1, 0, 0)
        glutSolidTeapot(1.5)
        glPopMatrix()

    def draw_o3dMesh(self):
        glPushMatrix()
        glScale(5, -5, -5)
        glBegin(GL_TRIANGLES)
        for i_surface, surface in enumerate(self.mesh_surfaces):
            #print("inside draw triangle:", i_surface, surface)
            x = 0
            #glNormal3fv(self.mesh_normals[i_surface])
            for vertex in surface:
                print("vertex", self.mesh_vertices[vertex])
                x+=1
                #glColor3fv(colors[x])
                glVertex3fv(self.mesh_vertices[vertex])
        glEnd()
        glPopMatrix()

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
        #gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0)

        # set viewport
        glViewport(0, 0, self.flipped_frame.shape[W_IDX], self.flipped_frame.shape[H_IDX])

        # you will have to set modelview matrix using extrinsic camera params
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0)
        if self.ret == True:
            glPushMatrix()
            #glTranslatef(-5, 2, 0)
            glLoadMatrixd(self.M)
            ########################################################################################################################
            # drawing routine
            # now that the camera params have been set, draw your 3D shapes
            # first, save the current matrix
            #print("Drawing Sphere")
            #self.draw_teapot(0, 0)
            #glutSolidSphere(0.2, 100, 100)
            self.drawFaceMesh(self.faceMeshPoints)
            #self.drawFaceMeshQuad(self.faceMeshPoints)
            #self.draw_o3dMesh()
            glPopMatrix()

        # show the rendering on the screen
        glutSwapBuffers()

        # post the next redisplay
        glutPostRedisplay()

    def idle(self):
        '''grabs a frame from the camera'''
        '''ret_frame, frame = self.vcap.read()

        if ret_frame == False:
            print('no frames to grab, exiting')
            sys.exit(0)

        self.frame = frame'''

        #while(True):
        #3D points is normalized(NDC) and 2D points is pixel coordinates
        _ret, self.frame, mediapipe_face_landmarks, opencv_face_coordinates = mpa.faceMesh(self.vcap) #calling the mediapipe api facemesh

        self.ret = _ret
        if _ret==True:
            #TODO: for multiple faces
            new_opencv_face_coordinates = opencv_face_coordinates[0]
            print("opencv coordinates: "+str(new_opencv_face_coordinates[467]))
            #parse landmarks from mediapipe
            mediapipe_face_landmarks = str(mediapipe_face_landmarks).split("landmark")
            mediapipe_face_landmarks = mediapipe_face_landmarks[1:len(mediapipe_face_landmarks)]
            new_mediapipe_face_landmarks = []
            for landmark in mediapipe_face_landmarks:
                landmark = landmark.split()
                new_mediapipe_face_landmarks.append([float(landmark[2]), float(landmark[4]), float(landmark[6])])
            self.faceMeshPoints = new_mediapipe_face_landmarks
            print("landmarks: "+str(new_mediapipe_face_landmarks[467]))

            #print(new_mediapipe_face_landmarks)
            '''fileObj = open('face_point_cloud.ply', 'w')
            for landmark in new_mediapipe_face_landmarks:
                fileObj.write(str(landmark[0])+" "+str(landmark[1])+" "+str(landmark[2])+"\n")
            fileObj.close()'''

            #Open3D mesh calculations
            '''points3d = np.array(new_mediapipe_face_landmarks)
            pcd = o3d.geometry.PointCloud()
            pcd.points = o3d.utility.Vector3dVector(points3d)
            pcd.estimate_normals()
            pcd.orient_normals_consistent_tangent_plane(100)
            alpha = 0.05
            mesh = o3d.geometry.TriangleMesh.create_from_point_cloud_alpha_shape(pcd, alpha)
            mesh.compute_triangle_normals()
            self.mesh_vertices = np.array(mesh.vertices)
            self.mesh_surfaces = np.array(mesh.triangles)
            self.mesh_normals = np.array(mesh.triangle_normals)
            print("self.mesh_vertices: ",self.mesh_vertices.shape)
            print("self.mesh_surfaces: ",self.mesh_surfaces.shape)
            print("self.mesh_normals: ",self.mesh_normals.shape)'''

            # storing object points and image points for frame
            objp = np.zeros((6, 3), dtype=np.float64)
            objp[0] = np.array([new_mediapipe_face_landmarks[0][0], new_mediapipe_face_landmarks[0][1], new_mediapipe_face_landmarks[0][2]])
            objp[1] = np.array([new_mediapipe_face_landmarks[250][0], new_mediapipe_face_landmarks[250][1], new_mediapipe_face_landmarks[250][2]])
            objp[2] = np.array([new_mediapipe_face_landmarks[350][0], new_mediapipe_face_landmarks[350][1], new_mediapipe_face_landmarks[350][2]])
            objp[3] = np.array([new_mediapipe_face_landmarks[450][0], new_mediapipe_face_landmarks[450][1], new_mediapipe_face_landmarks[450][2]])
            objp[4] = np.array([new_mediapipe_face_landmarks[150][0], new_mediapipe_face_landmarks[150][1], new_mediapipe_face_landmarks[150][2]])
            objp[5] = np.array([new_mediapipe_face_landmarks[50][0], new_mediapipe_face_landmarks[50][1], new_mediapipe_face_landmarks[50][2]])
            #print("objps: "+str(objp))

            # finding corners of the rect, image points
            corners = np.zeros((6, 2), dtype=np.float64)
            corners[0] = np.array([new_opencv_face_coordinates[0][0], new_opencv_face_coordinates[0][1]])
            corners[1] = np.array([new_opencv_face_coordinates[250][0], new_opencv_face_coordinates[250][1]])
            corners[2] = np.array([new_opencv_face_coordinates[350][0], new_opencv_face_coordinates[350][1]])
            corners[3] = np.array([new_opencv_face_coordinates[450][0], new_opencv_face_coordinates[450][1]])
            corners[4] = np.array([new_opencv_face_coordinates[150][0], new_opencv_face_coordinates[150][1]])
            corners[5] = np.array([new_opencv_face_coordinates[50][0], new_opencv_face_coordinates[50][1]])
            #print("corners: "+str(corners))

            # getting model view matrix
            # https://stackoverflow.com/questions/44375149/opencv-to-opengl-coordinate-system-transform
            ret_, rvec, tvec = cv2.solvePnP(objp, corners, self.calCam[1], self.calCam[2])
            #print(ret_, rvec, tvec)
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
            #print("M:"+str(M))
            self.M = np.array(M).T


def main(argv):
    if len(argv) == 2: #python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
        source = 0
    elif len(argv) == 3:
        source = argv[1]
    else:
        print("Add camera calib file to argv")
        sys.exit(0)

    face_relight = faceRelighting(source)
    face_relight.idle()

    # initialize GLUT
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
    # glutInitWindowPosition(20, 20)
    glutInitWindowSize(face_relight.width, face_relight.height)
    glutCreateWindow("Facemesh Relighting")

    #shading in OpenGL
    '''glEnable(GL_LIGHTING)
    lightZeroPosition = [20.,2.,-2.,1.]
    #lightZeroColor = [1.8,1.0,0.8,1.0]
    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition)
    #glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor)
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1)
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05)
    glColor(1.0, 0.0, 0.0)
    glEnable(GL_LIGHT0)'''

    glLight(GL_LIGHT0, GL_POSITION,  (1, 0, 0, 0)) # directional light from the front
    glLightfv(GL_LIGHT0, GL_AMBIENT, (0, 0, 0, 1))
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (1, 1, 1, 1))
    glEnable(GL_LIGHTING)
    glEnable(GL_LIGHT0)
    glEnable(GL_COLOR_MATERIAL)
    glEnable(GL_BLEND)
    #glBlendFunc(GL_SRC_ALPHA, GL_ONE)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glColor4f(1.0, 1.0, 1.0, 0.9)
    #glColor(1.0, 0.0, 0.0)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE )
    glEnable(GL_DEPTH_TEST)

    #depth in opengl
    #glDepthFunc(GL_LESS)
    #glEnable(GL_DEPTH_TEST)
    #glDepthMask(GL_FALSE)

    # set up GUI callback functions
    glutDisplayFunc(face_relight.display)
    glutKeyboardFunc(face_relight.keyboard)
    glutIdleFunc(face_relight.idle)

    # start GUI loop
    glutMainLoop()

if __name__ == '__main__':
    main(sys.argv)
