# SPDX-FileCopyrightText: 2023 Jim McKeown
# SPDX-License-Identifier: MIT
# Based on Adafruit example

import time
import numpy as np
from matplotlib import pyplot as plt
import adafruit_fingerprint
import serial

uart = serial.Serial("/dev/ttyACM0", baudrate=57600, timeout=1)

# put fingerprint box is in passthru mode
time.sleep(0.5)
uart.write(b'passthru\r')
time.sleep(2.0)

finger = adafruit_fingerprint.Adafruit_Fingerprint(uart)

def get_fingerprint():
    """Get a finger print image, template it, and see if it matches"""
    print("Waiting for image...")
    # read 3 x gives the user time to fully press the finger to the prism
    # and results in a much higher quality image - jjm
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass    
    print("Templating...")
    if finger.image_2_tz(1) != adafruit_fingerprint.OK:
        return False
    print("Searching...")
    if finger.finger_search() != adafruit_fingerprint.OK:
        return False
    return True

def get_fingerprint_photo():
    """Get and show fingerprint image"""
    print("Waiting for image...")
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    print("Got image...Transferring image data...")
    imgList = finger.get_fpdata('image', 2)
    imgArray = np.zeros(73728,np.uint8) 
    for i, val in enumerate(imgList):
        imgArray[(i * 2)] = (val & 240)
        imgArray[(i * 2) + 1] = (val & 15) * 16
    imgArray = np.reshape(imgArray, (288, 256))
    plt.title("Fingerprint Image")
    plt.imshow(imgArray)
    plt.show(block = False)

def get_fingerprint_model():
    """Get and show fingerprint model"""
    imgList = finger.get_fpdata('char', 1)
    imgArray = np.zeros(768,np.uint8) 
    
    for i, val in enumerate(imgList):
        imgArray[i] = (val)
    #imgArray = np.reshape(imgArray, (384, 2))
        
    imgArray = np.reshape(imgArray, (32, 24))
    plt.title("Fingerprint Model")
    plt.imshow(imgArray)
    plt.show(block = False)    

def get_fingerprint_preview():
    """Get a finger print image, show it, template it, and see if it matches!"""
    print("Waiting for image...")
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    while finger.get_image() != adafruit_fingerprint.OK:
        pass
    print("Got image...Transferring image data...")
    imgList = finger.get_fpdata('image', 2)
    imgArray = np.zeros(73728,np.uint8) 
    for i, val in enumerate(imgList):
        imgArray[(i * 2)] = (val & 240)
        imgArray[(i * 2) + 1] = (val & 15) * 16
    imgArray = np.reshape(imgArray, (288, 256))
    plt.title("Fingerprint Image")
    plt.imshow(imgArray)
    plt.show(block = False)
    print("Templating...")
    if finger.image_2_tz(1) != adafruit_fingerprint.OK:
        return False
    print("Searching...")
    if finger.finger_search() != adafruit_fingerprint.OK:
        return False
    return True    

# pylint: disable=too-many-statements
def enroll_finger(location):
    """Take a 2 finger images and template it, then store in 'location'"""
    for fingerimg in range(1, 3):
        if fingerimg == 1:
            print("Place finger on sensor...")
        else:
            print("Place same finger again...")

        while True:
            while finger.get_image() != adafruit_fingerprint.OK:
                pass
            while finger.get_image() != adafruit_fingerprint.OK:
                pass
            while finger.get_image() != adafruit_fingerprint.OK:
                pass
            i = finger.get_image()
            if i == adafruit_fingerprint.OK:
                print("Image taken...transferring image...")
                break
            if i == adafruit_fingerprint.NOFINGER:
                print(".", end="")
            elif i == adafruit_fingerprint.IMAGEFAIL:
                print("Imaging error")
                return False
            else:
                print("Other error")
                return False


        imgList = finger.get_fpdata('image', 2)
        imgArray = np.zeros(73728,np.uint8) 
        for i, val in enumerate(imgList):
            imgArray[(i * 2)] = (val & 240)
            imgArray[(i * 2) + 1] = (val & 15) * 16
        imgArray = np.reshape(imgArray, (288, 256))
        plt.title("Fingerprint Image")
        plt.imshow(imgArray)
        plt.show(block = False)
        
        print("Is this image accepable? (y/n)")
        c = input("> ")
        if (c != "y") and (c != "Y"):
            return False        

        print("Templating...", end="")
        i = finger.image_2_tz(fingerimg)
        if i == adafruit_fingerprint.OK:
            print("Templated")
        else:
            if i == adafruit_fingerprint.IMAGEMESS:
                print("Image too messy")
            elif i == adafruit_fingerprint.FEATUREFAIL:
                print("Could not identify features")
            elif i == adafruit_fingerprint.INVALIDIMAGE:
                print("Image invalid")
            else:
                print("Other error")
            return False

        if fingerimg == 1:
            print("Remove finger")
            time.sleep(1)
            while i != adafruit_fingerprint.NOFINGER:
                i = finger.get_image()

    print("Creating model...", end="")
    i = finger.create_model()
    if i == adafruit_fingerprint.OK:
        print("Created")
    else:
        if i == adafruit_fingerprint.ENROLLMISMATCH:
            print("Prints did not match")
        else:
            print("Other error")
        return False

    print("Storing model #%d..." % location, end="")
    i = finger.store_model(location)
    if i == adafruit_fingerprint.OK:
        print("Stored")
    else:
        if i == adafruit_fingerprint.BADLOCATION:
            print("Bad storage location")
        elif i == adafruit_fingerprint.FLASHERR:
            print("Flash storage error")
        else:
            print("Other error")
        return False

    return True

def get_num():
    """Use input() to get a valid number from 0 to 127. Retry till success"""
    i = -1
    while (i > 127) or (i < 0):
        try:
            i = int(input("Enter ID # from 0-127: "))
        except ValueError:
            pass
    return i

while True:
    print("----------------")
    if finger.read_templates() != adafruit_fingerprint.OK:
        raise RuntimeError("Failed to read templates")
    print("Fingerprint templates:", finger.templates)
    print("e) enroll print with preview")
    print("f) find print")
    print("d) delete print")
    print("v) view print")
    print("p) preview and find print")
    print("m) view print model")
    print("----------------")
    c = input("> ")

    if c == "e":
        enroll_finger(get_num())
    if c == "f":
        if get_fingerprint():
            print("Detected #", finger.finger_id, "with confidence", finger.confidence)
        else:
            print("Finger not found")
    if c == "d":
        if finger.delete_model(get_num()) == adafruit_fingerprint.OK:
            print("Deleted!")
        else:
            print("Failed to delete")
    if c == "v":
        get_fingerprint_photo()
    if c == "p":
        if get_fingerprint_preview():
            print("Detected #", finger.finger_id, "with confidence", finger.confidence)
        else:
            print("Finger not found")            
    if c == "m":
        get_fingerprint_model()



























