## Running the project
The project only works with the correct calibration file for your corresponding video or webcam (to get all the intrinsic AND extrinsic camera parameters).

1.) Create an intrinsic calibration file for a set of images using the following command:
''' python3 get_calibration.py folder-containing-images intrinsic-calibration-filename.pkl '''

This command will dump the output of calibrationCamera() into intrinsic-calibration-filename.pkl, and prints out the average projection error as well. 
An additional human readable intrinsic calibration output file will be saved as 'intrinsic-calibration-filename.txt' as well.

2.) Augment your camera or a video with OpenGL renderings by using the following commands:
''' python OpenGL_CV.py intrinsic-calibration-filename.pkl '''  -----> captures from webcam using intrinsic camera parameters generated from step 1
''' python OpenGL_CV.py output-video.mp4 intrinsic-calibration-filename.pkl ''' -----> captures from video file using intrinsic camera parameters generated from step 1
Remember to use the CORRECT  ".pkl" generated from step 1.

-----------------------------------------------------------------------------------------

## Assignment Questions and Answers
1.) How do the intrinsic calibration parameters of your camera compare to the camera for the image sequence we provided (e.g. argue which camera lens has a wider angle, which one more distortion, and distortion of what kind)?

Let image_sequence = image sequence provided.

Here are the intrinsic calibration results for the image_sequnece:
fovx: 52.137421858152884
fovy: 40.24065515575359
focalLength: 654.0386532237726
principalPoint: (327.3427021540153, 251.35785144971933)
aspectRatio: 1.0013571887206716
distortion parameters: [[ 0.05267651 -0.49561904  0.0052524   0.00314395  1.72472031]]

... and the intrinsic calibration results for my own:
fovx: 57.89891364585097
fovy: 40.48294062001526
focalLength: 976.183297674218
principalPoint: (533.5700894797187, 370.08907959128067)
aspectRatio: 0.9999950775879968
distortion parameters: [[ 0.03444774  0.10200284  0.00234118 -0.00425086 -0.55281377]]


Upon observation, my own vs the image_sequence have quite different focal lengths -- the focal length on the lens for mine is 937mm, whereas with the image_sequence, it is 654mm. 
The longer the focal length, the narrower the field of view is and the higher the magnification [3]. This implies that the image_sequence has a wider field of view and wider angle, and thus has more distortion than that of my own camera.
This is confirmed with the distortion parameters from the two videos. The image_sequence exhibits changes in distortion higher than my own camera (No distortions means the distortion coefficients would be 0.)
Additionally, according to OpenCV documentation on distortion coefficients, typically when k1 > 0, the lens exhibit barrel distortions [2]. Since my camera has positive distortions all around, this means that is has positive radial distortion (or barrel distortion). Likewise, since k1 > 0 for the image_sequence, it also exhibits barrel distortion.


Sources:
[1] OpenCV documentation: https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html#void%20projectPoints(InputArray%20objectPoints,%20InputArray%20rvec,%20InputArray%20tvec,%20InputArray%20cameraMatrix,%20InputArray%20distCoeffs,%20OutputArray%20imagePoints,%20OutputArray%20jacobian,%20double%20aspectRatio)
[2] Camera Calibration: https://www.mathworks.com/help/vision/ug/camera-calibration.html#bu0nj3f
[3] Understanding focal length: https://www.nikonusa.com/en/learn-and-explore/a/tips-and-techniques/understanding-focal-length.html

2.) How does the average error compare? Can you explain the difference in error?
The average pixel error is higher at (0.89) for the image sequence provided vs. the average pixel error of my own camera at (0.36).
The difference in error could be attributed to the image sequence provided having more photos at weird angles or being a bit more distorted as described in the first answer above, since the average pixel error is essentially measuring how accurate the distance is between the points in the original image plane and the points in the world plane.

Sources:
OpenCV documentation: https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html#void%20projectPoints(InputArray%20objectPoints,%20InputArray%20rvec,%20InputArray%20tvec,%20InputArray%20cameraMatrix,%20InputArray%20distCoeffs,%20OutputArray%20imagePoints,%20OutputArray%20jacobian,%20double%20aspectRatio)

-----------------------------------------------------------------------------------------

## Misc
- My own photos and the movie file taken from my camera are stored in the folder "dana_checker"
- My code contains the sources from various websites that helped me with some of the functions throughout this project (i.e. how to create projection matrix from stackoverflow)
- Example calibration files are in dana_checker and checkerboardFromVideo respectively
- (MacOS) To create a video (mp4) from images where each image is a frame, run the following:
(brew install ffmpeg must be done first)
''' ffmpeg -framerate 1 -pattern_type glob -i 'folder-with-images/*.jpg' -c:v libx264 -r 1 -pix_fmt yuv420p output-video.mp4 '''
