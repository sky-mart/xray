import sys
import numpy as np
import os
from scipy.misc import imread
from PIL import Image

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

def save_24bit(name, img_f32):
    img = Image.fromarray(img_f32)
    img = img.convert(mode="RGB")
    img.save(name)
    
def save_samples(samples, dirpath, base_name, extension=".png"):
    for ay in xrange(samples.shape[0]):
        for ax in xrange(samples.shape[1]):
            name = os.path.join(dirpath, base_name + ("_ay=%d_ax=%d" % (ay, ax)) + extension) 
            save_24bit(name, samples[ay, ax])

def usage():
    print "Usage:"
    print sys.argv[0] , "image", "psf_size", "noise_std"

if __name__ == "__main__":
    if len(sys.argv) != 4:
        usage()
    else:
        try:
            img_path = sys.argv[1]
            k = int(sys.argv[2])
            noise_std = float(sys.argv[3])

            img = imread(img_path, flatten=True)
            psf = np.ones((k, k)) / k**2

            print "Original shape:", img.shape
            img = cut_for_sampling(img, k)
            print "Shape used for sampling:", img.shape

            samples = generate_samples(img, psf)

            for i in xrange(samples.shape[0]):
                for j in xrange(samples.shape[1]):
                    samples[i][j] += np.random.normal(0, noise_std, samples[i][j].shape)

            dirpath = os.path.dirname(img_path)
            base_name_with_ext = os.path.basename(img_path)       
            dot_index = base_name_with_ext.rfind('.')
            base_name = base_name_with_ext[:dot_index]
            extension = base_name_with_ext[dot_index:]

            save_samples(samples, dirpath, base_name, extension)
            print "Successfully generated samples"
        except Exception as e:
            print e