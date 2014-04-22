#!/usr/bin/env python2

import serial
import threading
from time import sleep
step = 0.05
class CruiseControl(threading.Thread):
    ser = None
    stoprequest = None
    def __init__(self, x=90, y=90):
        super(CruiseControl, self).__init__()
        self.ser = serial.Serial('/dev/ttyUSB0', 9600)
        self.set_x = self.x = x
        self.set_y = self.y = y
        self.stoprequest = threading.Event()

    def run(self):
        while not self.stoprequest.isSet():
            print("x: %s y: %s"%(self.set_x,self.set_y))
            if self.x == self.set_x:
                pass
            elif self.x > self.set_x:
                self.set_x = self.set_x + 1
            else:
                self.set_x = self.set_x - 1

            if self.y == self.set_y:
                pass
            elif self.y > self.set_y:
                self.set_y = self.set_y + 1
            else:
                self.set_y = self.set_y - 1
            self.ser.write("%d,%d\n"%(self.set_x,self.set_y))
            self.ser.flush()
            sleep(step)
    def join(self, timeout=None):
        self.stoprequest.set()
        super(CruiseControl, self).join(timeout)
def main(args):
    thread = CruiseControl()
    thread.start()
    try:
        while True:
            thread.x = int(sys.stdin.readline())
            thread.y = int(sys.stdin.readline())
    except Exception as e:
        print(e)
        pass
    
    thread.join()
if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
