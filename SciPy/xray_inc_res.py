import numpy as np
from scipy.fftpack import fft2, ifft2
from scipy.misc import imread, imsave
from fractions import gcd
import matplotlib.pyplot as plt

from os import listdir
from parse import *
import os
import re

from PIL import Image

def upsample(img, n):
    # new_size is n times bigger
    new_shape = tuple(map(lambda x: x * n, img.shape))
    
    new_img = np.empty(new_shape)
    for i in xrange(img.shape[0]):
        for j in xrange(img.shape[1]):
            new_img[i*n:(i+1)*n, j*n:(j+1)*n] = np.full((n, n), img[i, j])
    return new_img

def deconv2(c, b):
    Nout = c.shape[0] - b.shape[0] + 1
    Mout = c.shape[1] - b.shape[1] + 1
    b = np.pad(b, [(0, c.shape[0] - b.shape[0]), (0, c.shape[1] - b.shape[1])], "constant", constant_values=0)
    
    a = ifft2(fft2(c) / fft2(b))
    a = np.real(a[:Nout, :Mout])
    return a

def adjsize(N, K):
    while gcd(N[0], K[0]) != 1:
        N = N[0] + 1, N[1] 
    while gcd(N[1], K[1]) != 1:
        N = N[0], N[1] + 1
    return N

def deconv2_adjsize(c, b):
    Nout = c.shape[0] - b.shape[0] + 1
    Mout = c.shape[1] - b.shape[1] + 1   
    N = adjsize(c.shape, b.shape)    
        
    c = np.pad(c, [(0, N[0] - c.shape[0]), (0, N[1] - c.shape[1])], "constant", constant_values=0)
    b = np.pad(b, [(0, N[0] - b.shape[0]), (0, N[1] - b.shape[1])], "constant", constant_values=0)
      
    a = ifft2(fft2(c) / fft2(b))
    a = np.real(a[:Nout, :Mout])
    return a

def wiener_deconv2(c, b, snr):
    Nout = c.shape[0] - b.shape[0] + 1
    Mout = c.shape[1] - b.shape[1] + 1
    b = np.pad(b, [(0, Nout-1), (0, Mout-1)], "constant", constant_values=0)
    
    Fc = fft2(c)
    Fb = fft2(b)
    
    G = (Fb * np.conj(Fb)) / (Fb * np.conj(Fb) + 1.0 / snr) / Fb
    a = ifft2(G * Fc)
    a = np.real(a[:Nout, :Mout])
    a[abs(a) < 1e-12] = 0.0
    return a

def wiener_deconv2_adjsize(c, b, snr):
    Nout = c.shape[0] - b.shape[0] + 1
    Mout = c.shape[1] - b.shape[1] + 1   
    N = adjsize(c.shape, b.shape)  
    
    c = np.pad(c, [(0, N[0] - c.shape[0]), (0, N[1] - c.shape[1])], "constant", constant_values=0)
    b = np.pad(b, [(0, N[0] - b.shape[0]), (0, N[1] - b.shape[1])], "constant", constant_values=0)

    Fc = fft2(c)
    Fb = fft2(b)
    
    G = (Fb * np.conj(Fb)) / (Fb * np.conj(Fb) + 1.0 / snr) / Fb
    a = ifft2(G * Fc)
    a = np.real(a[:Nout, :Mout])
    a[abs(a) < 1e-12] = 0.0
    return a

def wiener_deconv2_debug(c, b, snr):
    Nout = c.shape[0] - b.shape[0] + 1
    Mout = c.shape[1] - b.shape[1] + 1   
    N = adjsize(c.shape, b.shape)  
    
    c = np.pad(c, [(0, N[0] - c.shape[0]), (0, N[1] - c.shape[1])], "constant", constant_values=0)
    b = np.pad(b, [(0, N[0] - b.shape[0]), (0, N[1] - b.shape[1])], "constant", constant_values=0)

    Fc = fft2(c)
    Fb = fft2(b)
    
    num = Fb * np.conj(Fb)
    print "num\n", num
    den = Fb * np.conj(Fb) + 1.0 / snr
    print "den\n", den
    x = (Fb * np.conj(Fb)) / (Fb * np.conj(Fb) + 1.0 / snr)
    print "Fb*Fb' / (Fb*Fb' + 1.0 / snr)\n", x
    G = x / Fb
    print "G\n", G
    print "G*FC\n", G*Fc
    a = ifft2(G * Fc)
    print "a\n", a
    a = np.real(a[:Nout, :Mout])
    a[abs(a) < 1e-12] = 0.0
    return a

def show_pics(pics, titles):
    _, axis = plt.subplots(nrows=1, ncols=len(pics), sharex=True, sharey=True, figsize=(14,6))
    for i in xrange(len(pics)):
        axis[i].imshow(pics[i], cmap='Greys_r')
#         axis[i].axis('off')
        axis[i].set_title(titles[i])
    plt.show()

def cut_for_sampling(img, k):
    cut = img
    if img.shape[0] % k == 0:
        cut = img[:-(k - 1),:]
    elif img.shape[0] % k != 1:
        cut = img[:-((img.shape[0] % k) - 1),:]
    if cut.shape[1] % k == 0:
        cut = cut[:,:-(k - 1)]
    elif cut.shape[1] % k != 1:
        cut = cut[:,:-((img.shape[1] % k) - 1)]
    return cut

def generate_samples(img, psf):
    k = psf.shape[0]; # assume psf as square template
    
    # pad image with zeros
    padded = np.pad(img, [(k-1, k-1), (k-1, k-1)], "constant", constant_values=0)
    
    sample_rows, sample_cols = padded.shape[0] / k, padded.shape[1] / k
    samples = np.empty((k, k, sample_rows, sample_cols))  
    
    # anchor_y, anchor_x - coordinates in img, from where we start to attach the template (upper left corner)
    for anchor_y in xrange(k):
        for anchor_x in xrange(k):
            tmp = np.zeros((sample_rows, sample_cols))
            
            # y, x - coordinates where the template (upper left corner) is attached
            for y in xrange(anchor_y, padded.shape[0] - (k-1), k):
                for x in xrange(anchor_x, padded.shape[1] - (k-1), k):
                    i = (y - anchor_y) / k
                    j = (x - anchor_x) / k
                    tmp[i, j] = np.sum(padded[y:y+k, x:x+k] * psf)
            samples[anchor_y, anchor_x] = tmp
    return samples

def conv_from_samples(samples, to_shape=None):
    k, _, sample_rows, sample_cols = samples.shape # assume psf as square template
    if to_shape:
        sampled = np.empty(to_shape)
    else:
        sampled = np.empty((k*sample_rows, k*sample_cols))
                           
    for anchor_y in xrange(k):
        for anchor_x in xrange(k): 
            for y in xrange(anchor_y, sampled.shape[0], k):
                for x in xrange(anchor_x, sampled.shape[1], k):
                    i = (y - anchor_y) / k
                    j = (x - anchor_x) / k
                    sampled[y, x] = samples[anchor_y, anchor_x, i, j]
    return sampled

def save_24bit(name, img_f32):
    img = Image.fromarray(img_f32)
    img = img.convert(mode="RGB")
    img.save(name)
    
def save_samples(samples, path, base_name, extension=".png"):
    for ay in xrange(samples.shape[0]):
        for ax in xrange(samples.shape[1]):
            name = os.path.join(path, base_name + ("_ay=%d_ax=%d" % (ay, ax)) + extension) 
            save_24bit(name, samples[ay, ax])

def read_samples(path, base_name, extension=".png"):
    pattern = re.compile(base_name + "_ay=[0-9]+_ax=[0-9]+\\" + extension)
    samples_paths = [entry for entry in listdir(path) if pattern.match(entry)]
    k = int(len(samples_paths) ** 0.5) # assume the correct number of samples in the folder
    sample_shape = imread(os.path.join(path, samples_paths[0]), flatten=True).shape
    samples = np.empty((k, k) + sample_shape)
    for sample_name in samples_paths:
        r = parse(base_name + "_ay={:d}_ax={:d}" + extension, sample_name)
        (ay, ax) = r.fixed
        samples[ay, ax] = imread(os.path.join(path, sample_name), flatten=True)
    return samples