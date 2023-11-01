import cv2
import numpy as np
import math

# Load an image from a file
image = cv2.imread('test_image.jpg')

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

def getVertex(pCoord):
	#pass
	H = pCoord.x
	t = pCoord.y
	d = pCoord.z
	x = d*math.cos(t)
	y = d*math.sin(t)
	z = H
	return vertex(int(x),int(y),int(z))

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

theta = 0

meshPoints = []
lineLenth = []

# Check if the image was successfully loaded
if image is not None:
    # Display the image

    for i in range(2):
        image = cv2.imread('test_image.jpg')
        # print("i", meshPoints)
        tlp = (98.0,61.0)
        trp = (269.0,155.0)
        brp = (247.0,241.0)
        blp = (113.0,278.0)
        pts = np.array([tlp,trp,brp,blp])
        image = four_point_transform(image, pts)

        # cv2.imshow('transformed Image' + str(i), image)

        lowerb = np.array([100, 0, 0])
        upperb = np.array([255, 255, 255])
        #1200,1600
        red_line = cv2.inRange(image, lowerb, upperb)
        ##red_line = cv2.resize(red_line, (60,80), interpolation = cv2.INTER_AREA)

        # cv2.imshow("filtered" + str(i), red_line)

        h,w = np.shape(red_line)
        backG = np.zeros((h, w))

        # print(backG.shape)

        bottomR = 0

        r = 0
        for cIndex in np.argmax(red_line, axis=1):
            # print(cIndex)
            # print(red_line[r,cIndex], r, cIndex)
            if red_line[r,cIndex] != 0:
                backG[r,cIndex] = 1
                bottomR = r
            r += 1
        # print(bottomR)

        # cv2.imshow("background" + str(i), backG)

        tempV = []
        r = 0
        centerC = 420.0 #center column
        for cIndex in np.argmax(backG,axis=1):
            # print(backG[r, cIndex], r, cIndex)
            if(backG[r,cIndex] == 1):
                #intvi = 0
                # print(cIndex, centerC)
                H = r-bottomR
                dist = cIndex - centerC
                coord = vertex(H,np.radians(theta),dist)
                # print(coord.write())
                tempV.append(coord)
            r += 1
        
        # print("len(tempV) : ", len(tempV))
        intv = 50
        intv = len(tempV)//intv

        # print("intv : ", intv)

        # print("----> ", len(tempV), intv)

        if(len(tempV) != 0 and intv != 0):
            V = []
            V.append(tempV[0])

            for ind in range(1,len(tempV)-2):
                # print(ind, intv, ind % intv == 0)
                if(ind % intv == 0):
                    V.append(tempV[ind])

            V.append(tempV[(len(tempV)-1)])
            meshPoints.append(V)
            lineLenth.append(-1*len(V))
            print(lineLenth)
        
        # print(meshPoints)
        # print(lineLenth)

        for col in meshPoints:
            for c in col:
                # print(c.write())
                pass
    
    # print(meshPoints)
    print(meshPoints)
    shortest = len(meshPoints[np.argmax(lineLenth)])
    # print(shortest) # meshPoints[0]' ın uzunluğu
    # print(np.argmax(lineLenth))

    for line in meshPoints:
        while(len(line) > shortest):
            line.pop(len(line)-2)
    
    # print(len(meshPoints))

    points = []
    faces = []
    firstRow = []
    prevRow = []
    for index in range(1,len(meshPoints[0])+1):

        points.append(getVertex(meshPoints[0][index-1]))
        firstRow.append(index)
    
    # print(points)
    for point in points:
        print(point.write())
    # print(firstRow)
    # print(len(meshPoints))

    prevRow = firstRow
    for col in range(0,len(meshPoints)):
        # print(col, " col")
        if col != 0:
            indexS = prevRow[-1]
            # print(prevRow, prevRow[-1], indexS)
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

    # print(faces, points)

    filetowrite='3d.obj'
    with open(filetowrite, 'w') as file:
        for point in points:
            file.write(point.write() + "\n")
        for f in faces:
            file.write(f.write() + "\n")
        file.close()

    while True:
        # Check for a key press and store the key code
        key = cv2.waitKey(1) & 0xFF
        
        # Check if the 'x' button was pressed (key code 27)
        if key == 27:
            break

    # Close all OpenCV windows
    cv2.destroyAllWindows()
else:
    print('Image not loaded. Please check the file path.')
