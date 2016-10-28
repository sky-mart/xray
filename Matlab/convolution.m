src = imread('../images/peka.png');
src = im2double(src);

k = 6;
psf = ones(k,k) / k^2;

conv_sz = size(src) + k - 1;

% for every n: GCD(N,K) * GCD(N,n) != N
% consequently, GCD(N,K) == 1
N = conv_sz(1);
while 1
    if (gcd(N, k) == 1)
        src = padarray(src, N - conv_sz, 0, 'post');    
        break;
    end
    N = N + 1;
end

c = conv2(src, psf);

b = padarray(psf, size(c) - size(psf), 0, 'post');

Fc = fft2(c);
Fb = fft2(b);
a = ifft2(Fc ./ Fb);

rest = a(1:size(src,1), 1:size(src,2));

mean(mean(rest - src))