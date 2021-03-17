CS291 Suriya Dakshina Murthy

1. To call the calibration file, hardcoded the location of screenshots for calibration, output matrix written to calibration_matrix.yaml
python3 camera_calib.py

2. To call the OpenGL overlay on Video, copy camera matrix and distortion parameters to global parameters from above yaml file
python3 OpenGL_CV.py <location to video>

Write out the following intrinsic camera parameters in a small text file, with one parameter per line (some of these you will need in the OpenGL part of the assignment): horizontal field of view, vertical field of view, focal length (using 0.0f for both apertureWidth and apertureHeight), principal point, aspect ratio parameters fx and fy, and distortion parameters k1, k2, p1, and p2.

Input Video -
camera_matrix:
654.0386532237726   0.0                327.34270215404126
0.0                 654.9263071068098  251.69899149056258
0.0                 0.0                1.0
dist_coeff:
0.05267650832192627 0.4956190413924691 0.0052524038601472495 0.003143951311043761 1.7247203077112738

My webcam -
camera_matrix:
910.8843804968606  0.0                - 620.0830549034282
0.0                914.9719257777986  373.9797758912618
0.0                0.0                1.0
dist_coeff:
0.09016813588756717 -0.196871593424749 -0.005570823681104732 -0.0011444615644275139 -0.11395723224378385

1. How do the intrinsic calibration parameters of your camera compare to the camera for the image sequence we provided (e.g. argue which camera lens has a wider angle, which one more distortion, and distortion of what kind)?
fovy input video camera: 40.25106737735966
fovy my webcamera:       57.59344511521835 (Wider Angle)

Distortions:
Video Radial Distortions(k1, k2, k3):   0.05267650832192627 0.4956190413924691 1.7247203077112738   (Pincushion Distortions)
Webcam Radial Distortions(k1, k2, k3):  0.09016813588756717 -0.196871593424749 -0.11395723224378385 (Barrel Distortions)

Video Tangential Distortion(p1, p2):  0.0052524038601472495 0.003143951311043761  (positive tangential non-parallelism)
Webcam Tangential Distortion(p1, p2): -0.005570823681104732 -0.0011444615644275139(negative tangential non-parallelism)

2. How does the average error compare? Can you explain the difference in error?
Input Video total error:  0.12599056381329476
My webcam total error:    0.13471688971576962

I had lesser samples for my webcam and hence the intrinsic parameters(camera matrix and distortion parameters) were not very accurate and not able to remove distortions from a reprojection of 3D objects in 2D image and hence a slight increase in error.
