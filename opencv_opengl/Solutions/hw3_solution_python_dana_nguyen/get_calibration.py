from cv2 import cv2
import glob
import numpy as np
import math
import json
import pickle
import sys
from numpy.linalg import inv

# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((6*8, 3), np.float32)
objp[:, :2] = np.mgrid[0:8, 0:6].T.reshape(-1, 2)

# Arrays to store object points and image points from all the images.
objpoints3D = []  # 3d point in real world space
imgpoints2D = []  # 2d points in image plane.

images = glob.glob(sys.argv[1]+'/*.jpg')

for fname in images:
    img = cv2.imread(fname)

    # Find the chess board corners
    ret, corners = cv2.findChessboardCorners(img, (8, 6), None)

    # If found, add object points, image points (after refining them)
    if ret == True:
        objpoints3D.append(objp)
        imgpoints2D.append(corners)

print("getting intrinsic calibration")
calCam = cv2.calibrateCamera(
    objpoints3D, imgpoints2D, img.shape[::-1][1:], None, None)
print("output of intrinsic calibration")
print(calCam)
ret_calib, mtx, dist, rvecs, tvecs = calCam

with open(sys.argv[2], "wb") as output:
    pickle.dump(calCam, output)


calMatValues = cv2.calibrationMatrixValues(
    calCam[1], (img.shape[::-1][1], img.shape[::-1][2]), 0, 0)

headers = ["fovx", "fovy", "focalLength", "principalPoint", "aspectRatio"]
# horizontal field of view, vertical field of view, focal length(using 0.0f for both apertureWidth and apertureHeight), principal point, aspect ratio parameters fx and fy, and distortion parameters k1, k2, p1, and p2.
with open((sys.argv[2]).split(".")[0]+".txt", "w") as output_txt:
    for idx, val in enumerate(calMatValues):
        output_txt.write(headers[idx]+": " + str(val))
        output_txt.write("\n")

    output_txt.write("distortion parameters: " + str(dist)+"\n")

print("doing avg pixel error")
avg_pixel_error = 0
total_pt = 0
reprojection_error = 0
# https://stackoverflow.com/questions/23781089/opencv-calibratecamera-2-reprojection-error-and-custom-computed-one-not-agree
for i in range(len(objpoints3D)):
    imgpoints2D2, _ = cv2.projectPoints(
        objpoints3D[i], rvecs[i], tvecs[i], mtx, dist)
    err = np.sum(np.abs(imgpoints2D[i]-imgpoints2D2)**2)
    reprojection_error += err
    total_pt += len(objpoints3D[i])

avg_pixel_error = np.sqrt(reprojection_error/total_pt)
# print(total_pt)
print("total reprojection error: ", reprojection_error)
print("average pixel error: ", avg_pixel_error)
print("mtx: ", mtx)
print("dist: ", dist)

cv2.destroyAllWindows()
