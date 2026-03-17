function bandwidth = occupiedBWFSK(symbolRate, order, modIndex)
    % only works for FSK
    if (order == 2)
        bandwidth = symbolRate * (modIndex + 1);
    elseif (order == 4)
        bandwidth = symbolRate * (3*modIndex + 1);
    end
end