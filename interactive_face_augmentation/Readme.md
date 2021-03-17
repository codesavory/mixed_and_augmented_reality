Face Detection - Haar Cascade Filter(cv2.CascadeClassifier())
2D to 3D - Converted the 2D points detected using above face detection to 3D using solvePnP
Spinning Teapots - added lighting and material and using glColorMask() and depth test occluded two planes
Gesture - Current gesture tracking using averaging with background subtraction

cmd - python3 interactive_face_augmentation.py intrinsic-calib-suriya.pkl
python3 file_name.py camera_intrinsics.pkl

Challenges -
1. Doing horizontal inversion mapping still working on it
2. 2D to 3D - directly mapped to object points of size 000 100 110 010, still working on using real FaceXYZ values
3. Spinning Teapot - To improve shading parameters(lighting and material)
4. Gesture Detection - Using manual background tracking worked, yet to add optical flow motion detection

References -
https://pysource.com/2018/05/17/background-subtraction-opencv-3-4-with-python-3-tutorial-32/
https://answers.opencv.org/question/171484/reverse-projection-from-2d-to-3d-i-have-intrinsic-parameter-of-camera-but-no-extrinsic-parameter-i-do-have-multiple-images-form-same-camera/
https://stackoverflow.com/questions/44375149/opencv-to-opengl-coordinate-system-transform
https://math.stackexchange.com/questions/2237994/back-projecting-pixel-to-3d-rays-in-world-coordinates-using-pseudoinverse-method
