import cv2

# Callback function for mouse events
def click_event(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        print(f'Coordinates: ({x}, {y})')

# Specify the path to the image file
image_path = 'imgs/four_points/0.jpg'  # Replace with the actual path to your image file

# Read the image using OpenCV
image = cv2.imread(image_path)

# Check if the image is successfully loaded
if image is not None:
    # Display the image
    cv2.imshow('Image', image)

    # Set the callback function for mouse events
    cv2.setMouseCallback('Image', click_event)

    # Wait for a key press and then close the window
    cv2.waitKey(0)
    cv2.destroyAllWindows()
else:
    print(f"Error: Unable to load the image at {image_path}")
