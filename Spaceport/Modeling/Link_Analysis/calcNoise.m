function noise = calcNoise(symbolRate, order, modIndex, noiseFig)
    % may need to make this higher for high temp at spaceport
    % i.e. 273 + 40 = 313
    T0 = 290; % K
    systemNoisePower = physconst("Boltzmann") * T0 * occupiedBW(symbolRate, order, modIndex);

    systemNoiseTemp = (systemNoisePower / occupiedBW(symbolRate, order, modIndex)) * (1 / physconst("Boltzmann"));

    % add noise of everything else
    noise = 10*log10(systemNoisePower*1000) + noiseFig + systemNoiseTemp;

end