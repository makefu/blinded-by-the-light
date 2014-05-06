#!/usr/bin/env python

import cv,serial
from cruise_control import CruiseControl


class Target:

    def __init__(self):
        self.capture = cv.CaptureFromCAM(1)
        cv.NamedWindow("Target", 1)

        self.cruise = CruiseControl()
        self.cruise.start()

    def run(self):
        # Capture first frame to get size
        frame = cv.QueryFrame(self.capture)
        frame_size = cv.GetSize(frame)
        color_image = cv.CreateImage(cv.GetSize(frame), 8, 3)
        grey_image = cv.CreateImage(cv.GetSize(frame), cv.IPL_DEPTH_8U, 1)
        moving_average = cv.CreateImage(cv.GetSize(frame), cv.IPL_DEPTH_32F, 3)

        first = True

        while True:
            closest_to_left = cv.GetSize(frame)[0]
            closest_to_right = cv.GetSize(frame)[1]

            color_image = cv.QueryFrame(self.capture)

            # Smooth to get rid of false positives
            #cv.Smooth(color_image, color_image, cv.CV_GAUSSIAN, 3, 0)
            brightness=00
            average_val=0.050
            scale_val=1.0
            thresh=40
            #dilate=18
            #erode=10
            dilate=18
            erode=10
            # manual exposure : 70
            #cv.AddWeighted(color_image,1,color_image,1,brightness,color_image)

            if first:
                difference = cv.CloneImage(color_image)
                temp = cv.CloneImage(color_image)
                cv.ConvertScale(color_image, moving_average, 1.0, 0.0)
                first = False
            else:
                cv.RunningAvg(color_image, moving_average,average_val , None)
                cv.RunningAvg(color_image, moving_average, 0.020, None)

            # Convert the scale of the moving average.
            cv.ConvertScale(moving_average, temp, scale_val, 0.0)

            # Minus the current frame from the moving average.
            cv.AbsDiff(color_image, temp, difference)

            # Convert the image to grayscale.
            cv.CvtColor(difference, grey_image, cv.CV_RGB2GRAY)

            # Convert the image to black and white.
            
            cv.Threshold(grey_image, grey_image, thresh, 255,
                    cv.CV_THRESH_BINARY)
            # brighten up
            # dilate -> helle flecken werden ausgeweitet
            
            cv.Dilate(grey_image, grey_image, None, dilate)

            # erode -> dunkle stellen werden ausgeweitet
            cv.Erode(grey_image, grey_image, None, erode)

            storage = cv.CreateMemStorage(0)
            contour = cv.FindContours(grey_image, storage, cv.CV_RETR_CCOMP, cv.CV_CHAIN_APPROX_SIMPLE)
            points = []

            while contour:
                bound_rect = cv.BoundingRect(list(contour))
                contour = contour.h_next()

                pt1 = (bound_rect[0], bound_rect[1])
                pt2 = (bound_rect[0] + bound_rect[2], bound_rect[1] + bound_rect[3])
                points.append(pt1)
                points.append(pt2)
                cv.Rectangle(color_image, pt1, pt2, cv.CV_RGB(255,0,0), 1)

            if len(points):
                center_point= reduce(lambda a, b: ((a[0] + b[0]) / 2, 
                    (a[1] + b[1]) / 2 ), points)
                x = center_point[0]
                y = center_point[1]
                # only for X
                #  boundaries
                if 100 < x < 640:
                    import serial
                    import numpy
                    #  interpolate from pixel to 0-1024
                    # we get some value for x between 640 and 0
                    x_val = numpy.interp(x,[0,640],[1024,0])
                    y_val = numpy.interp(y,[0,480],[1024,0])
                    print ("Original X/Y         - %d:%d"%(x,y))
                    print ("Interpolated Value   - %d:%d"%(x_val,y_val))
                    self.cruise.x = x_val
                    self.cruise.y = y_val
                    # send to serial crappy 

                cv.Circle(color_image, center_point, 40, cv.CV_RGB(255, 255, 255), 1)
                cv.Circle(color_image, center_point, 30, cv.CV_RGB(255, 100, 0), 1)
                cv.Circle(color_image, center_point, 20, cv.CV_RGB(255, 255, 255), 1)
                cv.Circle(color_image, center_point, 10, cv.CV_RGB(255, 100, 0), 1)

            cv.ShowImage("Target", color_image)

            # Listen for ESC key
            c = cv.WaitKey(7) % 0x100
            if c == 27:
                break

if __name__=="__main__":
    t = Target()
    t.run()
