# -*- coding: utf-8 -*-
"""
Created on Tue Oct 18 13:46:37 2016

@author: Autex
"""

import processinglib

src = face(gray=True)
k = 3
psf = np.ones((k, k)) / k**2
direct_conv = conv2(src, psf)
direct_rest = deconv2(direct_conv, psf)
show_pics([src, direct_rest], ["Source", "Directly restored"])