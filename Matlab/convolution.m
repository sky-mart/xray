src = imread('../images/peka.png');
src = im2double(src);

k = 4;
psf = ones(k,k) / k^2;

convolved = conv2(src, psf);

% T = convmtx2(psf, size(src));
% 
% c2 = reshape(T*src(:), size(psf) + size(src) -1);

convolution_size = size(src) + size(psf);
c = padarray(convolved, [1 1], 0, 'post');
%c = padarray(convolved, size(psf) - 1);
% c(convolution_size(1), convolution_size(2)) = 0;

% if mod(k,2) == 0
%     pre = int(k / 2);
%     post = int(k / 2);
% else
%     pre = int(k / 2);
%     post = int(k / 2) + 1;
% end

b = padarray(psf, [5, 5], 0, 'pre');
b = padarray(b, [5, 5], 0, 'post');
%b = padarray(psf, size(src) - 1);
% b(convolution_size(1), convolution_size(2)) = 0;

Fc = fft2(c);
Fb = fft2(b);
a = ifft2(Fc ./ Fb);

restored_size = convolution_size - size(psf) + 1;
rest = a(1:restored_size(1), 1:restored_size(2));

mean(mean(rest - src))