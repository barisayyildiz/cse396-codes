# ECE 5725
# Michael Xiao (mfx2) and Thomas Scavella (tbs47)
# 3D scanner software

import cv2
import numpy as np
import math
from time import sleep
import RPi.GPIO as GPIO
import time
import os
import wiringpi

# pins of the stepper motor
STEPPER_PIN_1 = 1
STEPPER_PIN_2 = 7
STEPPER_PIN_3 = 8
STEPPER_PIN_4 = 25

STEP_PER_MOVEMENT = 512
DELAY_ONE_STEP = 0.003
stepNumber = 0

# placeholder variable to track status of stepper motor
i=0

# GPIO setup
# GPIO.setmode(GPIO.BCM)
# GPIO.setup(STEPPER_PIN_1,GPIO.OUT)
# GPIO.setup(STEPPER_PIN_2,GPIO.OUT)
# GPIO.setup(STEPPER_PIN_3,GPIO.OUT)
# GPIO.setup(STEPPER_PIN_4,GPIO.OUT)
wiringpi.wiringPiSetup()
wiringpi.pinMode(STEPPER_PIN_1, 1)
wiringpi.pinMode(STEPPER_PIN_1, 1)
wiringpi.pinMode(STEPPER_PIN_1, 1)
wiringpi.pinMode(STEPPER_PIN_1, 1)

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

# # steps motor forward indicated number of steps
# # inputs x: number of steps to move, i: status of stepper
# # outputs new i (status of stepper)
# def step(x, i):
#     positive=0
#     negative=0
#     y=0

#     GPIO.output(out1,GPIO.LOW)
#     GPIO.output(out2,GPIO.LOW)
#     GPIO.output(out3,GPIO.LOW)
#     GPIO.output(out4,GPIO.LOW)
#     #x = input()
#     if x>0 and x<=400:
#       #print "step"
#       for y in range(x,0,-1):
#         if negative==1:
#           if i==7:
#             i=0
#           else:
#             i=i+1
#           y=y+2
#           negative=0
#         positive=1
#         #print((x+1)-y)
#         if i==0:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==1:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==2:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==3:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==4:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==5:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==6:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==7:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         if i==7:
#           i=0
#           continue
#         i=i+1


#     elif x<0 and x>=-400:
#       x=x*-1
#       for y in range(x,0,-1):
#         if positive==1:
#           if i==0:
#             i=7
#           else:
#             i=i-1
#           y=y+3
#           positive=0
#         negative=1
#         #print((x+1)-y)
#         if i==0:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==1:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==2:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==3:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.HIGH)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==4:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.LOW)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==5:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.HIGH)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==6:
#           GPIO.output(out1,GPIO.LOW)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         elif i==7:
#           GPIO.output(out1,GPIO.HIGH)
#           GPIO.output(out2,GPIO.LOW)
#           GPIO.output(out3,GPIO.LOW)
#           GPIO.output(out4,GPIO.HIGH)
#           time.sleep(0.03)
#           #time.sleep(1)
#         if i==0:
#           i=7
#           continue
#         i=i-1

#     return i

LOW = 0
HIGH = 1
def oneStep():
	global stepNumber
	if stepNumber == 0:
		wiringpi.digitalWrite(STEPPER_PIN_1, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_2, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_3, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_4, HIGH)
	elif stepNumber == 1:
		wiringpi.digitalWrite(STEPPER_PIN_1, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_2, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_3, HIGH)
		wiringpi.digitalWrite(STEPPER_PIN_4, LOW)
	elif stepNumber == 2:
		wiringpi.digitalWrite(STEPPER_PIN_1, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_2, HIGH)
		wiringpi.digitalWrite(STEPPER_PIN_3, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_4, LOW)
	elif stepNumber == 3:
		wiringpi.digitalWrite(STEPPER_PIN_1, HIGH)
		wiringpi.digitalWrite(STEPPER_PIN_2, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_3, LOW)
		wiringpi.digitalWrite(STEPPER_PIN_4, LOW)
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

	while(theta < 360):
		#will loop this
		cap = cv2.VideoCapture(0)
		ret, img = cap.read()
		sleep(1)
		cap.release()

		#get perspective
		tlp = (236.0,185.0)
		trp = (880.0,324.0)
		brp = (873.0,682.0)
		blp = (245.0,857.0)
		pts = np.array([tlp,trp,brp,blp])
		img = four_point_transform(img, pts)

		#---------- Preview the PERSPECTIVE picture ----------------
		#cv2.imshow("perspective", img)
		#cv2.waitKey(0)


		# filter
		lowerb = np.array([50, 0, 0])
		upperb = np.array([255, 255, 255])
		#1200,1600
		red_line = cv2.inRange(img, lowerb, upperb)
		##red_line = cv2.resize(red_line, (60,80), interpolation = cv2.INTER_AREA)


		#---------- Preview the filtered picture ----------------
		# cv2.imshow("perspective", red_line)
		# cv2.waitKey(0)
		#print red_line.shape
		cv2.imwrite(f"imgs/{counter}.jpg", red_line)
		print("counter: ", counter)
		counter += 1


		h,w = np.shape(red_line)
		backG = np.zeros((h, w))

		#print backG

		bottomR = 0

		r = 0
		for cIndex in np.argmax(red_line, axis=1):
			if red_line[r,cIndex] != 0:
				backG[r,cIndex] = 1
				bottomR = r
			r += 1

		#---------- Preview the processed picture ----------------
		#cv2.imshow("perspective", backG)
		#cv2.waitKey(0)


		tempV = []
		r = 0
		centerC = 420.0 #center column
		for cIndex in np.argmax(backG,axis=1):
			if(backG[r,cIndex] == 1):
				#intvi = 0
				H = r-bottomR
				dist = cIndex - centerC
				coord = vertex(H,np.radians(theta),dist)
				tempV.append(coord)
			r += 1

		# vertical resolution
		intv = 20
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
		move(STEP_PER_MOVEMENT)
		print("theta: ", theta)
		theta = theta + ((360.0 / (2048 / STEP_PER_MOVEMENT)))


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
			prevRow = currentRow

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
