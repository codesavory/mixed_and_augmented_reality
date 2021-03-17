

This is a second sample solution for Homework 1. A few notes about this solution:

Again, you should be able to compile the program with 
> cmake . 
> make

There are a few ways to run the program. Specific details can be found in the main function of the file assignment1.cpp, but here is an overview:

1) Calibrate via image set, video is from a file. Calibrating via image set requires you to create a txt file that contains the filenames of calibration images (one per line) and pass this as a command line parameter.
./assignment1 filelist.txt ../checkerboard.avi 

2) Calibrate via image set, use live video.
./assignment1 filelist.txt 

Options 1 & 2 also work with a text file with intrinsic camera parameters in the HW1 style instead of a text file that lists the calibration images. The program distinguishes the files by checking if the first character is an ASCII digit. If yes, it is assumed to be a camera parameter file, else a file with names of calibration images. E.g: 
./assignment1 params.txt ../checkerboard.avi 
./assignment1 params.txt 

3) Calibrate via live video, use live video. When calibrating via live video you must hold the chessboard up to the screen and press the 'c' button on your keyboard to take a picture. Take 10 pictures with the chessboard (move it around to different positions/orientations) in view to create your image calibration set. Once you have enough pictures, the AR overlay should automatically begin.

Second, there is a class CameraHelper that we have created that abstracts camera parameters. This class includes many functions that are useful for initializing, setting, and retrieving camera (i.e. intrinsic) parameters and calculating values dependent on these parameters (i.e. extrinsic params). This class is by no means comprehensive for everything you will need in the class for the whole quarter, but gives an idea about modularization possibilities and is sufficient for this assignment.

