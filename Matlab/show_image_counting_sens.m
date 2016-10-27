function x = show_image_counting_sens(img, inv_sens)
    x = im2double(img);
    x = x .* inv_sens;
    imshow(x);
end