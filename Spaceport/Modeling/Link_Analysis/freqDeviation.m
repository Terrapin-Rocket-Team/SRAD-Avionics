function fdev = freqDeviation(symbolRate, modIndex)
    % if 2FSK returns outer deviation
    % if 4FSK returns inner deviation
    fdev = modIndex * symbolRate / 2;
end