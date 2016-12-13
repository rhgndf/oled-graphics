#!/usr/bin/python2
from time import sleep
import Adafruit_SSD1306
from PIL import Image
import sys
import cv2
import time

video = cv2.VideoCapture(sys.argv[1])

RST = 24

disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)

disp.begin()

to_skip = 0
success = 1

while success:
    start_time = time.time();
    success, cv2im = video.read()
    if success == 0:
        continue
    if to_skip > 0:
       to_skip = to_skip - 1
       continue

    cv2im = cv2.cvtColor(cv2im, cv2.COLOR_BGR2RGB)
    image = Image.fromarray(cv2im).convert('1')
    disp.image(image)
    disp.display()
    elapsed = time.time() - start_time
    time.sleep(max(0, (1./30) - elapsed))
    to_skip += max(elapsed - (1./30), 0) * 30
