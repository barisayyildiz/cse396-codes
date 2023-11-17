# ECE 5725
# Michael Xiao (mfx2) and Thomas Scavella (tbs47)
# 3D scanner software

import cv2
import numpy as np
import math
from picamera import PiCamera
from time import sleep
import RPi.GPIO as GPIO
import time
import os

# pins of the stepper motor
STEPPER_PIN_1 = 1
STEPPER_PIN_2 = 7
STEPPER_PIN_3 = 8
STEPPER_PIN_4 = 25

GPIO.setmode(GPIO.BCM)
control_pins = [1,7,8,25]                                                                                                                                                                          
for pin in control_pins:
  GPIO.setup(pin, GPIO.OUT)
  GPIO.output(pin, 0)

STEP_PER_MOVEMENT = 32
DELAY_ONE_STEP = 0.003
stepNumber = 0

# placeholder variable to track status of stepper motor
i=0


#vertex class
class vertex:
	def __init__(self, x,y,z):
		self.x = x
		self.y = y
		self.z = z

	def write(self):
		return "v " + str(self.x) + " " + str(self.y) + " " +str(self.z)
		#return  str(self.x) + "," + str(self.y) + "," +str(self.z)
#face class
class face:
	def __init__(self, v1,v2,v3):
		self.v1 = v1
		self.v2 = v2
		self.v3 = v3

	def write(self):
		return "f " + str(self.v1) + " " + str(self.v2) + " " +str(self.v3)

def order_points(pts):
	# initialzie a list of coordinates that will be ordered
	# such that the first entry in the list is the top-left,
	# the second entry is the top-right, the third is the
	# bottom-right, and the fourth is the bottom-left
	rect = np.zeros((4, 2), dtype = "float32")
	# the top-left point will have the smallest sum, whereas
	# the bottom-right point will have the largest sum
	s = pts.sum(axis = 1)
	rect[0] = pts[np.argmin(s)]
	rect[2] = pts[np.argmax(s)]
	# now, compute the difference between the points, the
	# top-right point will have the smallest difference,
	# whereas the bottom-left will have the largest difference
	diff = np.diff(pts, axis = 1)
	rect[1] = pts[np.argmin(diff)]
	rect[3] = pts[np.argmax(diff)]
	# return the ordered coordinates
	return rect

def four_point_transform(image, pts):
	# obtain a consistent order of the points and unpack them
	# individually
	rect = order_points(pts)
	(tl, tr, br, bl) = rect
	# compute the width of the new image, which will be the
	# maximum distance between bottom-right and bottom-left
	# x-coordiates or the top-right and top-left x-coordinates
	widthA = np.sqrt(((br[0] - bl[0]) ** 2) + ((br[1] - bl[1]) ** 2))
	widthB = np.sqrt(((tr[0] - tl[0]) ** 2) + ((tr[1] - tl[1]) ** 2))
	maxWidth = max(int(widthA), int(widthB))
	# compute the height of the new image, which will be the
	# maximum distance between the top-right and bottom-right
	# y-coordinates or the top-left and bottom-left y-coordinates
	heightA = np.sqrt(((tr[0] - br[0]) ** 2) + ((tr[1] - br[1]) ** 2))
	heightB = np.sqrt(((tl[0] - bl[0]) ** 2) + ((tl[1] - bl[1]) ** 2))
	maxHeight = max(int(heightA), int(heightB))
	# now that we have the dimensions of the new image, construct
	# the set of destination points to obtain a "birds eye view",
	# (i.e. top-down view) of the image, again specifying points
	# in the top-left, top-right, bottom-right, and bottom-left
	# order
	dst = np.array([
		[0, 0],
		[maxWidth - 1, 0],
		[maxWidth - 1, maxHeight - 1],
		[0, maxHeight - 1]], dtype = "float32")
	# compute the perspective transform matrix and then apply it
	M = cv2.getPerspectiveTransform(rect, dst)
	warped = cv2.warpPerspective(image, M, (maxWidth, maxHeight))
	# return the warped image
	return warped

# transforms cylindrical coordinates into rectangular coordinates
def getVertex(pCoord):
	#pass
	H = pCoord.x
	t = pCoord.y
	d = pCoord.z
	x = d*math.cos(t)
	y = d*math.sin(t)
	z = H
	return vertex(int(x),int(y),int(z))

stepNumber = 0
def oneStep():
	global stepNumber
	if stepNumber == 0:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 1)
	elif stepNumber == 1:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 1)
		GPIO.output(control_pins[3], 0)
	elif stepNumber == 2:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 1)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 0)
	elif stepNumber == 3:
		GPIO.output(control_pins[0], 1)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 0)
	stepNumber += 1
	if stepNumber > 3:
		stepNumber = 0
	return stepNumber

def move(stepPrecision):
	precisionCounter = 0
	while precisionCounter < stepPrecision:
		oneStep()
		precisionCounter += 1
		sleep(DELAY_ONE_STEP)

while (1):
	# angular resolution
	numItt = 20

	# angle
	theta = 0
	thetaInc = 360.0/numItt

	# motor position
	motorPos = 0
	motorPosI = 400.0/numItt

	#data
	meshPoints = []
	lineLenth = []
	counter = 0

	while(theta <= 360):
		#will loop this
		# cap = cv2.VideoCapture(0)
		# ret, img = cap.read()
		# sleep(0.2)
		# cap.release()
		camera = PiCamera()
		camera.start_preview()
		sleep(0.2)
		camera.capture('lineDetection.jpg')
		camera.close()
		img = cv2.imread('lineDetection.jpg')

		cv2.imwrite(f"imgs/original/{counter}.jpg", img)

		#get perspective
		tlp = (277.0,90.0)
		trp = (733.0,90.0)
		brp = (733.0,634.0)
		blp = (277.0,634.0)
		pts = np.array([tlp,trp,brp,blp])
		img = four_point_transform(img, pts)

		cv2.imwrite(f"imgs/four_points/{counter}.jpg", img)

		#---------- Preview the PERSPECTIVE picture ----------------
		#cv2.imshow("perspective", img)
		#cv2.waitKey(0)


		# # filter
		# lowerb = np.array([0, 0, 80])
		# upperb = np.array([255, 255, 255])
		# #1200,1600
		# red_line = cv2.inRange(img, lowerb, upperb)
		# ##red_line = cv2.resize(red_line, (60,80), interpolation = cv2.INTER_AREA)
		# cv2.imwrite(f"imgs/red_line/{counter}.jpg", red_line)

		h,w, _ = np.shape(img)
		backG = np.zeros((h, w))
		bottomR = 0

		rows, cols, _ = img.shape
		
		for r in range(rows):
			cIndex = np.argmax(img[r, :, 2])
			if img[r, cIndex, 2] > 35:
				backG[r, cIndex] = 1
				bottomR = r

		# for rIndex in range(h):
		# 	maxVal = -1
		# 	maxColIndex = -1
		# 	for cIndex in range(w):
		# 		val = img[rIndex, cIndex, 2]
		# 		if val > 50 and val > maxVal:
		# 			maxVal = val
		# 			maxColIndex = cIndex
		# 	if maxVal != -1:
		# 		backG[rIndex, maxColIndex] = 1
		# 		bottomR = rIndex
		cv2.imwrite(f"imgs/redline/{counter}.jpg", backG)

		# r = 0
		# for cIndex in np.argmax(red_line, axis=1):
		# 	if red_line[r,cIndex] != 0:
		# 		backG[r,cIndex] = 1
		# 		bottomR = r
		# 	r += 1
		
		# cv2.imwrite(f"imgs/final/{counter}.jpg", backG)

		#---------- Preview the processed picture ----------------
		#cv2.imshow("perspective", backG)
		#cv2.waitKey(0)


		tempV = []
		r = 0
		centerC = 300.0 #center column TODO ne olduğunu anla
		for cIndex in np.argmax(backG,axis=1):
			if(backG[r,cIndex] == 1):
				#intvi = 0
				H = r-bottomR
				dist = cIndex - centerC
				coord = vertex(H,np.radians(theta),dist)
				tempV.append(coord)
			r += 1

		# vertical resolution
		intv = 80
		intv = len(tempV)//intv

		if(len(tempV) != 0 and intv != 0):
			V = []
			V.append(tempV[0])

			for ind in range(1,len(tempV)-2):
				if(ind % intv == 0):
					V.append(tempV[ind])

			V.append(tempV[(len(tempV)-1)])
			meshPoints.append(V)
			print(str(len(V)))
			lineLenth.append(-1*len(V))

		# theta += thetaInc
		# i = step(int(motorPosI),i)
		# time.sleep(0.3)
		print("theta: ", theta)
		move(STEP_PER_MOVEMENT)
		theta = theta + ((360.0 / (2048 / STEP_PER_MOVEMENT)))
		counter += 1


	# for row in meshPoints:
	# 	for coord in row:
	# 		print getVertex(coord).write()


	shortest = len(meshPoints[np.argmax(lineLenth)])
	# print shortest

	for line in meshPoints:
		while(len(line) > shortest):
			line.pop(len(line)-2)


	points = []
	faces = []
	firstRow = []
	prevRow = []
	lastVertices = []
	for index in range(1,len(meshPoints[0])+1):

		points.append(getVertex(meshPoints[0][index-1]))
		firstRow.append(index)

	prevRow = firstRow
	for col in range(0,len(meshPoints)):
		if col != 0:
			indexS = prevRow[-1]
			currentRow = []
			for point in range(0,len(meshPoints[col])-1):
				tl = indexS + point + 1
				bl = tl + 1
				tr = prevRow[point]
				br = prevRow[point + 1]

				f1 = face(tl,tr,bl)
				f2 = face(bl,tr,br)
				faces.append(f1)
				faces.append(f2)

				points.append(getVertex(meshPoints[col][point]))
				currentRow.append(tl)
				if(point == len(meshPoints[col])-2):
					points.append(getVertex(meshPoints[col][point+1]))
					currentRow.append(bl)

				if col == (len(meshPoints)-1):
					tr = tl
					br = bl
					tl = firstRow[point]
					bl = firstRow[point+1]
					f1 = face(tl,tr,bl)
					f2 = face(bl,tr,br)
					faces.append(f1)
					faces.append(f2)
			lastVertices.append(prevRow[-1])
			prevRow = currentRow
		
	
	# objenin tabanını çiz
	for i in range(1, len(lastVertices)-1):
		faces.append(face(lastVertices[0], lastVertices[i], lastVertices[i+1]))

	#---------- debugging prints ----------------
	# for point in points:
	# 	print(point.write())
	# for face in faces:
	# 	print(face.write())

	# writing the file
	filetowrite='3d.obj'
	with open(filetowrite, 'w') as file:
		for point in points:
			file.write(point.write() + "\n")
		for f in faces:
			file.write(f.write() + "\n")
		file.close()
	break


GPIO.cleanup()
