
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Edit these values only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Analysis specifications
margin = 20; % dB
sensitivity = -95; % dBm
BER_target = 10^-3; 

% signal settings
f = 0.2225e9; % hz
bitrate = 100e3; % bits per second, should be divisible by 2
mod_index_2FSK = 1;
mod_index_4FSK = 1;
BW_2FSK = occupiedBWFSK(bitrate, 2, mod_index_2FSK);
BW_4FSK = occupiedBWFSK(bitrate/2, 4, mod_index_4FSK);
BW_BPSK = occupiedBWPSK(bitrate);
BW_QPSK = occupiedBWPSK(bitrate/2);

% ground station location
GS_alt = 1.8288; % meters, AGL (Spaceport data)
GS_lat = 32.940012; % degrees (Spaceport data)
GS_long = -106.920470; % degrees (Spaceport data)

% ground station specs
pointing_angle_error = 5; % degrees
pad_height = 10; % ft

% hardware specs
TX_ant_eff = 0.50; % percent
TX_ant_gain = 0; % dBi
TX_ant_type = "linear"; % linear, RHCP, or LHCP
TX_pow = 28; % dBm
RX_ant_eff = 0.60; % percent
RX_ant_gain = 0; % dBi
LNA_gain = 28; % dB
LNA_P1dB = 28; % dBm
RX_ant_type = "RHCP"; % linear, RHCP, or LHCP
antennaNoiseTemp = 50; % K


componentNoiseFigures = [1.2,   2.1]; % dB
%                        LNA1   Radio LNA
componentGains = [28,  1]; % dB
%                 LNA1  RadioLNA
additionalLosses = [0.35, 3]; % dBm
%                   SW

% FEC settings
doFECSim = false;
N = 255; % block size
k = 245; % data size
flightTime = 60; % seconds

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

close all;

addedComponentNoise = 0;
additionalLoss = 0;

% get total noise
for l = 1:length(componentNoiseFigures)
    addedComponentNoise = addedComponentNoise + componentNoiseFigures(l);
end

% get total losses
for l = 1:length(additionalLosses)
    additionalLoss = additionalLoss + additionalLosses(l);
end

Pr = calcReceivedPower(f, GS_lat, GS_long, GS_alt, TX_ant_eff, TX_ant_gain, TX_ant_type, TX_pow, ...
    RX_ant_eff, RX_ant_gain, RX_ant_type, LNA_gain, LNA_P1dB, pointing_angle_error, pad_height, 3, "SACFlightData", "SACOrientQuat");
Pr = Pr - additionalLoss;

calcMargin = min(Pr) - sensitivity - margin;
if (calcMargin > 0)
    fprintf("Pass base sensitivity check with margin: %f dB\n", calcMargin + margin)
else
    fprintf("Fail base sensitivity check with margin: %f dB\n", calcMargin + margin)
end

flightData = table2array(struct2table(load('SACFlightData.mat')));
rocket_alt = table2array(flightData(:,1).*3);

figure(1)
grid on;
hold on;
yyaxis left;
ylabel("Power (dBm)")
plot(0:0.1:(length(Pr)-1)/10, Pr, 'LineWidth',3)
yyaxis right;
plot(0:0.1:(length(Pr)-1)/10, rocket_alt, "--k", 'LineWidth',3)
ylabel("Altitude (ft)")
ax = gca;
ax.YAxis(1).Color = 'k';
ax.YAxis(2).Color = 'k';
ylim([0, inf])
xlabel("time (s)")
title("Received Power Over Time")
legend("Received Power", "Rocket altitude")

% calcs for 2FSK
N_2FSK = calcNoise(BW_2FSK, componentNoiseFigures, componentGains, antennaNoiseTemp);
SNR_2FSK = Pr - N_2FSK;
EbNo_2FSK = SNR_2FSK + 10*log10(BW_2FSK/bitrate);

BER_2FSK_awgn = berawgn(EbNo_2FSK, "fsk", 2, "noncoherent");
BER_2FSK_fading = berfading(EbNo_2FSK,"fsk", 2, 1, "noncoherent");

% calcs for 4FSK
N_4FSK = calcNoise(BW_4FSK, componentNoiseFigures, componentGains, antennaNoiseTemp);
SNR_4FSK = Pr - N_4FSK;
EbNo_4FSK = SNR_4FSK + 10*log10(BW_4FSK/bitrate);

BER_4FSK_awgn = berawgn(EbNo_4FSK, "fsk", 4, "noncoherent");
BER_4FSK_fading = berfading(EbNo_4FSK,"fsk", 4, 1, "noncoherent");

% calcs for BPSK
% N_BPSK = calcNoise(BW_BPSK, componentNoiseFigures, componentGains, antennaNoiseTemp);
% SNR_BPSK = Pr - N_BPSK;
% EbNo_BPSK = SNR_BPSK + 10*log10(BW_BPSK/bitrate);
% 
% BER_BPSK_awgn = berawgn(EbNo_BPSK, "psk", 2, "diff");
% BER_BPSK_fading = berfading(EbNo_BPSK,'dpsk',2,1); % assume worst case Rician fading

% calcs for QPSK
% N_QPSK = calcNoise(BW_QPSK, componentNoiseFigures, componentGains, antennaNoiseTemp);
% SNR_QPSK = Pr - N_QPSK;
% EbNo_QPSK = SNR_QPSK + 10*log10(BW_QPSK/bitrate);
% 
% BER_QPSK_awgn = berawgn(EbNo_QPSK, "psk", 4, "diff");
% BER_QPSK_fading = berfading(EbNo_QPSK,'dpsk',4,1);

figure(2)
grid on;
plot(0:0.1:(length(Pr)-1)/10, SNR_2FSK, 'LineWidth',3)
hold on;
plot(0:0.1:(length(Pr)-1)/10, SNR_4FSK, 'LineWidth',3)
xlabel("Time (s)")
ylabel("SNR (dBm)")
title("SNR vs time")
legend("Estimated SNR 2FSK", "Estimated SNR 4FSK")

figure(3)
grid on;
semilogy(0:0.1:(length(Pr)-1)/10, BER_BPSK_awgn)
hold on;
semilogy(0:0.1:(length(Pr)-1)/10, BER_QPSK_awgn)
yline(BER_target, 'LineWidth',3)
xlabel("Time (s)")
ylabel("BER (errors/total)")
title("BER vs time")
legend("Estimated AWGN BER 2FSK", "Estimated AWGN BER 4FSK", "BER target")
axis([0, (length(Pr)-1)/10, 10^-20, 10^0])

figure(4)
grid on;
semilogy(0:0.1:(length(Pr)-1)/10, BER_2FSK_fading, 'LineWidth',3)
hold on;
semilogy(0:0.1:(length(Pr)-1)/10, BER_4FSK_fading, 'LineWidth',3)
yline(BER_target, 'LineWidth',3)
xlabel("Time (s)")
ylabel("BER (errors/total)")
title("BER vs time")
legend("Estimated Rician Fading BER 2FSK", "Estimated Rician Fading BER 4FSK", "BER target")
axis([0, (length(Pr)-1)/10, 10^-20, 10^0])

% sweep Eb/N0 to find required power
EbNo_sweep = 1:0.01:50;
% BER_sweep_BPSK = berfading(EbNo_sweep,'dpsk',2,1);
% BER_sweep_QPSK = berfading(EbNo_sweep,'dpsk',4,1);
BER_sweep_2FSK = berfading(EbNo_sweep,"fsk", 2, 1, "noncoherent");
BER_sweep_4FSK = berfading(EbNo_sweep,"fsk", 4, 1, "noncoherent");

Preq_2FSK = 0;
Preq_4FSK = 0;
for k=1:length(EbNo_sweep)
    if (BER_sweep_2FSK(k) <= BER_target && Preq_2FSK == 0)
        BER_sweep_2FSK(k)
        BER_target
        Preq_2FSK = EbNo_sweep(k) - 10*log10(BW_2FSK/bitrate) + N_2FSK;
    end
    if (BER_sweep_4FSK(k) <= BER_target && Preq_4FSK == 0)
        Preq_4FSK = EbNo_sweep(k) - 10*log10(BW_4FSK/bitrate) + N_4FSK;
    end
end

calcMargin_2FSK = min(Pr) - Preq_2FSK - margin;
fprintf("\nRequired power for 2FSK: %f\n", Preq_2FSK)
if (calcMargin_2FSK > 0)
    fprintf("Pass with margin: %f dB\n", calcMargin_2FSK + margin)
else
    fprintf("Fail with margin: %f dB\n", calcMargin_2FSK + margin)
end

calcMargin_4FSK = min(Pr) - Preq_4FSK - margin;
fprintf("\nRequired power for 4FSK: %f\n", Preq_4FSK)
if (calcMargin_4FSK > 0)
    fprintf("Pass with margin: %f dB\n", calcMargin_4FSK + margin)
else
    fprintf("Fail with margin: %f dB\n", calcMargin_4FSK + margin)
end

figure(5)
hold on;
grid on;
ylabel("Margin (dB)")
plot(0:0.1:(length(Pr)-1)/10, Pr - Preq_4FSK, 'LineWidth',3)
% plot(0:0.1:(length(Pr)-1)/10, Pr - Preq_QPSK, 'LineWidth',3)
yline(margin, 'LineWidth',3)
ylim([0, inf])
xlabel("time (s)")
title("Link margin")
legend("Link margin 4FSK", "Margin target")

if (doFECSim)
    %errors_2FSK_awgn = simRS(bitrate, flightTime, N, k, BER_2FSK_awgn);
    errors_2FSK_fading = simRS(bitrate, flightTime, N, k, BER_2FSK_fading);
    %errors_4FSK_awgn = simRS(bitrate, flightTime, N, k, BER_4FSK_awgn);
    errors_4FSK_fading = simRS(bitrate, flightTime, N, k, BER_4FSK_fading);

    figure()
    plot(errors_2FSK_fading(:, 1), errors_2FSK_fading(:, 2))
    hold on;
    plot(errors_2FSK_fading(:, 1), errors_2FSK_fading(:, 3))
    xlabel("Time (s)")
    ylabel("Bit errors")
    title("Bit errors vs time")
    legend("Raw Errors 2FSK Rician Fading", "Received Errors 2FSK Rician Fading")
    figure()
    plot(errors_2FSK_fading(:, 1), errors_4FSK_fading(:, 2))
    hold on;
    plot(errors_2FSK_fading(:, 1), errors_4FSK_fading(:, 3))
    xlabel("Time (s)")
    ylabel("Bit errors")
    title("Bit errors vs time")
    legend("Raw Errors 4FSK Rician Fading", "Received Errors 4FSK Rician Fading")
    figure()
    plot(errors_2FSK_fading(:, 1), errors_2FSK_fading(:, 4))
    hold on;
    plot(errors_2FSK_fading(:, 1), errors_4FSK_fading(:, 4))
    xlabel("Time (s)")
    ylabel("Symbol errors")
    title("Symbol errors vs time")
    legend("Symbol Errors 2FSK Rician Fading", "Symbol Errors 4FSK Rician Fading")

end