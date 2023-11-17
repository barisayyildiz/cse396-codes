import cv2
from picamera import PiCamera

# Open the default camera (you can specify a different camera index if needed)

# Function for mouse events
def click_event(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        # Convert the coordinates back to the original image size
        true_x = int(x / scale_factor)
        true_y = int(y / scale_factor)
        print(f'True Coordinates: ({true_x}, {true_y})')

# Set the callback function for mouse events
cv2.namedWindow('Resized Camera Feed')
cv2.setMouseCallback('Resized Camera Feed', click_event)

# Specify the desired width for resizing
target_width = 800

while True:
    # Read a frame from the camera
    # ret, frame = cap.read()
    # camera = PiCamera()
    # camera.start_preview()
    # sleep(0.2)
    # camera.capture('lineDetection.jpg')
    # camera.close()
    # frame = cv2.imread('lineDetection.jpg')
    frame = cv2.imread(f"imgs/original/{0}.jpg")

    # Calculate the scale factor
    scale_factor = target_width / frame.shape[1]

    # Resize the frame
    resized_frame = cv2.resize(frame, (target_width, int(frame.shape[0] * scale_factor)))

    # Display the resized frame
    cv2.imshow('Resized Camera Feed', resized_frame)

    # Break the loop if 'q' is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the camera and close all windows
cv2.destroyAllWindows()
