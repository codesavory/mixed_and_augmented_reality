Realtime Face Relighting using OpenCV, OpenGL, mediapipe, Open3D

Takes the input video frame using OpenCV and calculates 3D mesh points using mediapipe, this 3D point cloud is sent to Open3D to calculate normals and do surface reconstruction, this surface is then relit using virual lights in openGL and overlaid on the original face using a solvepnp() tranform with a transparency blend

Libraries - pip install mediapipe
pip install open3d

cmd -
python3 <program_name> <camera_Calibration_file>
python3 realtimeFaceRelighting.py intrinsic-calib-suriya.pkl

Website - https://docs.google.com/document/d/e/2PACX-1vQryCLg0MJVZhnu19qQ1SakHQMn0BWfSIRSxXwK9lXiuQUnomwxPcVkbjFj8jlJFKFXTN9U0i5lVvGa/pub

References -
https://google.github.io/mediapipe/
http://www.open3d.org/docs/latest/introduction.html
http://pyopengl.sourceforge.net/documentation/index.html
https://docs.opencv.org/master/index.html
