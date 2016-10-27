im_num = 22;
height = 790;
width = 524;

images = zeros(im_num, height, width);

% find average image
im = imread('..\\images\\sens_check\\im01.png');
images(1,:,:) = im2double(im);
aver_im = images(1,:,:); 
for i = 2:im_num
    im_str = sprintf('..\\images\\sens_check\\im%02d.png', i);
    im = imread(im_str);
    images(i,:,:) = im2double(im);
    aver_im = aver_im + images(i,:,:);
end
aver_im = aver_im / im_num;
aver_im = squeeze(aver_im);

% calculate sensitivity
aver_pixel = mean(mean(aver_im));
sens = aver_im / aver_pixel;

inv_sens = ones(size(sens));
inv_sens(sens == 0) = 0;
sens(sens == 0) = 1;
inv_sens = inv_sens ./ sens;
imwrite(inv_sens, '..\\images\\sens_check\\inv_sens.png');

aver_mean = 0;
aver_std = 0;
for i = 1:im_num
    for j = 1:height
        for k = 1:width
            images(i,j,k) =  images(i,j,k) * inv_sens(j,k);
        end
    end
    aver_mean = aver_mean + mean(mean(images(i,:,:)));
    aver_std  = aver_std  + std(std(images(i,:,:)));
end
aver_mean = aver_mean / im_num
aver_std = aver_std / im_num





