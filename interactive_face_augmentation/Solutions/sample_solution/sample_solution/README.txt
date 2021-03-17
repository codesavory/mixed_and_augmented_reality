Building
--------------------
$ cmake .
$ make


Running
--------------------
$ ./opengl_cv [video_file.avi] < params_sample.txt

Note: the included params_sample.txt contains the calibration values for the two
sample videos. The file is in the same format used by Assignment 1.


Controls
--------------------
'f': Toggle between optical flow and background subtraction. The initial motion
detection method is optical flow.

'b': Use the current frame as the background. This must be used at least once
before background subtraction can be enabled.

'g': Display grayscale image of optical flow/background subtraction.

'r': Toggle display of yellow rectangle (OpenCV 2D face position) and red
points (3D positioned and projected with OpenGL).

'o': Toggle display of 3D objects.

'q': Quit.



To calculate the depth of the face, we use the property of similar triangles 
formed by the image plane and a point on the face. Solving for the Y 
world-space coordinate of a point on the face (although X would also work), we 
find that we know all measurements of the smaller triangle containing the image 
plane but do not know the world-space coordinate of the face point or its 
distance, Z. However, we do know another point is located at the same Z 
coordinate the height of the face away. We can use these two sets of similar 
triangles to solve a linear system and determine the Z coordinate of the face 
points. The X and Y coordinates can then be unprojected from screen space by 
rearranging the basic projection equation.
