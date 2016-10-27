N_max = 15;

for N = 12:N_max
    for K = 2:N
        a1 = zeros(1, K-1);
        a2 = zeros(1, K-1);
        for n = 0:N-1
            for i = 1:K-1
                a1(i) = K * mod(n*i, N);
                a2(i) = N * i;
            end
            if isempty(setdiff(a1, a2))
%                 cond1 = (K == N) && (gcd(N, n) == 1);
%                 cond2 = K * gcd(N, n) == N;
                %if ~(K * gcd(N, n) == N)
                    disp([N, K, n]);
                %end
            end
        end
    end
end