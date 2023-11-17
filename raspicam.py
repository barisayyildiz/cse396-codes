from picamera import PiCamera
from time import sleep

camera = PiCamera()

# Adjust camera settings if needed
# camera.resolution = (1920, 1080)

camera.start_preview()
sleep(5)  # Allow the camera to adjust to light conditions
camera.capture('image.jpg')
camera.stop_preview()
