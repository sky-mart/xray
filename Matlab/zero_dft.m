

for M = 2:100
    s = 0;
    for m = 0:M-1
        s = s + exp(2*pi*m*1i/M);
    end
    disp(s);
end