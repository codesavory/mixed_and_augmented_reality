
CS290I Assignment 1


BUILDING AND RUNNING
==============================
The two parts of the project are in two separate directores:
> Camera Calibation: calibration/
    - build: cmake . && make
    - run: ./calibration checkerboard_frames/*.png > params.txt

> OpenGL Augmentation: opengl/
    - build: cmake . && make
    - run: ./opengl_cv checkerboard.avi < params.txt
    - controls: [space]: switch overlay; [q]: quit.

You can also run the entire process with:
    ./calibration *.png | ./opengl_cv checkerboard.avi


SOURCES
==============================
http://opencv.itseez.com/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html
The OpenCV library documentation.

http://tech.groups.yahoo.com/group/OpenCV/message/5650
Use of glFrustum: This post from the OpenCV mailing list might help with determining the
clipping plane positions from my camera matrix.

http://www.songho.ca/opengl/gl_transform.html
For more detail on transformation matrices.

http://opengl.org/
Miscellaneous OpenGL functions.

http://cs.ucsb.edu/~holl/CS291A
The lecture slides.
