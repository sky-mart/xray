

% for M = 2:100
%     s = 0;
%     for m = 0:M-1
%         s = s + exp(2*pi*m*1i/M);
%     end
%     disp(s);
% end

N = 15;
K = 6;
n = 5;

for m = 0:N-1
    s1 = 0;
    for k = 0:K-1
        s1 = s1 + exp(1i*2*pi*n*k/N);
    end
    s2 = 0;
    for l = 0:K-1
        s2 = s2 + exp(1i*2*pi*l*m/N);
    end
    s = s1*s2;
    disp([m, s1, s2, s])
end

