import cv2
import mediapipe as mp
from typing import List, Tuple, Union
import math

mp_drawing = mp.solutions.drawing_utils
mp_face_mesh = mp.solutions.face_mesh

def _normalized_to_pixel_coordinates(
    normalized_x: float, normalized_y: float, image_width: int,
    image_height: int) -> Union[None, Tuple[int, int]]:
  """Converts normalized value pair to pixel coordinates."""

  # Checks if the float value is between 0 and 1.
  def is_valid_normalized_value(value: float) -> bool:
    return (value > 0 or math.isclose(0, value)) and (value < 1 or
                                                      math.isclose(1, value))

  if not (is_valid_normalized_value(normalized_x) and
          is_valid_normalized_value(normalized_y)):
    # TODO: Draw coordinates even if it's outside of the image bounds.
    return None
  x_px = min(math.floor(normalized_x * image_width), image_width - 1)
  y_px = min(math.floor(normalized_y * image_height), image_height - 1)
  return x_px, y_px

def faceMesh(cap):
    # For webcam input:
    drawing_spec = mp_drawing.DrawingSpec(thickness=1, circle_radius=1)
    #cap = cv2.VideoCapture(0)
    mediapipe_meshes = []
    opencv_coordinates = []
    with mp_face_mesh.FaceMesh(
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5) as face_mesh:
      while cap.isOpened():
        success, image = cap.read()
        if not success:
          print("Ignoring empty camera frame.")
          # If loading a video, use 'break' instead of 'continue'.
          continue

        # Flip the image horizontally for a later selfie-view display, and convert
        # the BGR image to RGB.
        image = cv2.cvtColor(cv2.flip(image, 1), cv2.COLOR_BGR2RGB)
        # To improve performance, optionally mark the image as not writeable to
        # pass by reference.
        image.flags.writeable = False
        results = face_mesh.process(image)
        #print("face_mesh.process(image): "+str(len(results.multi_face_landmarks)))

        # Draw the face mesh annotations on the image.
        image.flags.writeable = True
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
        check_face_found = False
        #TODO: add check for multi_face_landmarks
        if results.multi_face_landmarks:
          check_face_found = True
          for face_landmarks in results.multi_face_landmarks:
            mediapipe_meshes.append(face_landmarks)
            '''mp_drawing.draw_landmarks(
                image=image,
                landmark_list=face_landmarks,
                connections=mp_face_mesh.FACE_CONNECTIONS,
                landmark_drawing_spec=drawing_spec,
                connection_drawing_spec=drawing_spec)'''
            image_rows, image_cols, _ = image.shape
            idx_to_coordinates = {}
            for idx, landmark in enumerate(face_landmarks.landmark):
                landmark_px = _normalized_to_pixel_coordinates(landmark.x, landmark.y,
                                                       image_cols, image_rows)
                if landmark_px:
                  idx_to_coordinates[idx] = landmark_px

            opencv_coordinates.append(idx_to_coordinates)
            #draw circles in landmarks in OpenCV
            #for landmark_px in idx_to_coordinates.values():
            #    cv2.circle(image, landmark_px, 1, (0, 255, 0), 1) #radius=1, color=green, thickness=1

        return check_face_found, image, mediapipe_meshes, opencv_coordinates
        #cv2.imshow('MediaPipe FaceMesh', image)
        #if cv2.waitKey(5) & 0xFF == 27:
          #break
    #cap.release()
