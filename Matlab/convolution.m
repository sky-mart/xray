src = imread('../images/peka.png');
src = im2double(src);

k = 6;
psf = ones(k,k) / k^2;

conv_sz = size(src) + k - 1;

if mod(conv_sz(1),k) == 0
    src = padarray(src, [1, 0], 0, 'post');
end
if mod(conv_sz(2),k) == 0
    src = padarray(src, [0, 1], 0, 'post');
end

c = conv2(src, psf);

b = padarray(psf, size(c) - size(psf), 0, 'post');

Fc = fft2(c);
Fb = fft2(b);
a = ifft2(Fc ./ Fb);

rest = a(1:size(src,1), 1:size(src,2));

mean(mean(rest - src))