function loss = pointingLoss(lambda, ant_eff, ant_gain, delta_theta)
    % lambda in m, ant_eff is percent, ant_gain in dB, delta_theta in
    % degrees

    % find antenna diameter assuming dish antenna
    % may or may not work for other antenna types
    % though likely loss will be negligible
    D = lambda/pi*sqrt(1/ant_eff*10^(ant_gain/10));
    theta_3DB = 70*lambda/D;
    loss = -12*(delta_theta/theta_3DB)^2;
end