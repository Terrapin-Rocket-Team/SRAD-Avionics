function noise = calcNoise(bw, noiseFigs, gains, TA)
    % may need to make this higher for high temp at spaceport
    % i.e. 273 + 40 = 313
    T0 = 290; % K
    noiseFactors = 10.^(noiseFigs./10);
    powerGains = 10.^(gains./10);
    F = 0;
    for k=1:length(noiseFactors)
        G = 1;
        for l=1:(k-1)
            G = G * powerGains(l);
        end
        F = F + noiseFactors(k)/G;
    end
    TE = T0 * (F - 1); % K
    TS = TA + TE; % K
    systemNoisePower = physconst("Boltzmann") * TS * bw; % W

    % convert to dBm
    noise = 10*log10(systemNoisePower*1000);

end