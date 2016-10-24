src = imread('../images/peka.png');
src = im2double(src);

k = 3;
psf = ones(k,k) / k^2;

convolved = conv2(src, psf);
% T = convmtx2(psf, size(src));
% 
% c2 = reshape(T*src(:), size(psf) + size(src) -1);


% if mod(size(convolved,1), k) == 0
%     c = padarray(convolved, [1, 1], 0, 'post');
% else
    c = convolved;
% end

disp(size(c) - size(psf));
b = padarray(psf, size(c) - size(psf), 0, 'post');

% b = padarray(psf, [4, 4], 0, 'pre');
% b = padarray(b, [9, 9], 0, 'post');
%b = padarray(psf, size(src) - 1);
% b(convolution_size(1), convolution_size(2)) = 0;

Fc = fft2(c);
Fb = fft2(b);
a = ifft2(Fc ./ Fb);

restored_size = size(c) - size(psf);
rest = a(1:size(src,1), 1:size(src,2));

mean(mean(rest - src))