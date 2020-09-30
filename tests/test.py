#!/usr/bin/python3

import numpy as np
import cv2
import matplotlib.pyplot as plt

# import os
# os.system('')
# subprocess.run

print('Testing in Python')

im = cv2.imread("../../ARRI_AlexaDrums_3840x2160p_24_12b_P3_444_00000.ppm", cv2.IMREAD_UNCHANGED );
hist, bin_edges = np.histogram(im.astype('int32'), bins=range(4096));
_ = plt.hist(hist, bin_edges);
