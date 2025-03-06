
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Edit these values only
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% values for 30K

% signal settings
f = 433e6; % hz
bitrate = 500e3; % bits per second, should be divisible by 2

% ground station location
GS_alt = 0; % meters, AGL
GS_lat = 32.940012; % degrees
GS_long = -106.920470; % degrees

% hardware specs
TX_ant_eff = 0.80; % percent
TX_ant_gain = 0; % dBi
TX_ant_type = "linear"; % linear, RHCP, or LHCP
TX_pow = 33; % dBm
RX_ant_eff = 0.70; % percent
RX_ant_gain = 6; % dBi
RX_ant_type = "RHCP"; % linear, RHCP, or LHCP


componentNoiseFigures = [4.8, 0.35, 2]; % dBm
%                        PA    SW  Extra
additionalLosses = []; % dBm

% FEC settings
N = 255; % block size
k = 245; % data size
flightTime = 310; % seconds

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

close all;

% get symbol rates
symbolRate2FSK = bitrate;
symbolRate4FSK = bitrate/2;

addedComponentNoise = 0;
additionalLoss = 0;

% get total noise
for l = 1:length(componentNoiseFigures)
    addedComponentNoise = addedComponentNoise + componentNoiseFigures(l);
end

for l = 1:length(additionalLosses)
    additionalLoss = additionalLoss + additionalLosses(l);
end

Pr = calcReceivedPower(f, GS_lat, GS_long, GS_alt, TX_ant_eff, TX_ant_gain, TX_ant_type, TX_pow, ...
    RX_ant_eff, RX_ant_gain, RX_ant_type, 3, "SACFlightData", "SACOrientQuat");
Pr = Pr - additionalLoss;

figure(1)
hold on;
plot(1:length(Pr), Pr)
yline(-97)
yline(-88)
yline(-109)
xlabel("time (s*10)")
ylabel("received power (dBm)")
title("Received power")
legend("fspl+polloss circ", "min 500kbps", "min 1Mbps", "min telem")

% calcs for 2FSK
N_2FSK = calcNoise(symbolRate2FSK, 2, 1, addedComponentNoise);
SNR_2FSK = Pr - N_2FSK;
EbNo_2FSK = SNR_2FSK + 10*log10(occupiedBW(symbolRate2FSK, 2, 1)/bitrate);

BER_2FSK_awgn = berawgn(EbNo_2FSK, "fsk", 2, "coherent");
BER_2FSK_fading = berfading(EbNo_2FSK,'fsk',2,1,"coherent",0,1); % assume worst case Rician fading

% calcs for 4FSK
N_4FSK = calcNoise(symbolRate4FSK, 4, 1/3, addedComponentNoise);
SNR_4FSK = Pr - N_4FSK;
EbNo_4FSK = SNR_4FSK + 10*log10(occupiedBW(symbolRate4FSK, 4, 1/3)/bitrate);

BER_4FSK_awgn = berawgn(EbNo_4FSK, "fsk", 4, "coherent");
BER_4FSK_fading = berfading(EbNo_4FSK,'fsk',4,1,"noncoherent",0,1);

figure(2)
plot(1:length(Pr), SNR_2FSK)
hold on;
plot(1:length(Pr), SNR_4FSK)
xlabel("time (s*10)")
ylabel("SNR (dBm)")
title("SNR vs time")
legend("Estimated SNR 2FSK", "Estimated SNR 4FSK")

figure(3)
semilogy(1:length(Pr), BER_2FSK_awgn)
hold on;
semilogy(1:length(Pr), BER_4FSK_awgn)
xlabel("time (s*10)")
ylabel("BER (errors/total)")
title("BER vs time")
legend("Estimated AWGN BER 2FSK", "Estimated AWGN BER 4FSK")
axis([0, length(Pr), 10^-20, 10^0])

%%errors_2FSK_awgn = simRS(bitrate, flightTime, N, k, BER_2FSK_awgn);
errors_2FSK_fading = simRS(bitrate, flightTime, N, k, BER_2FSK_fading);
%%errors_4FSK_awgn = simRS(bitrate, flightTime, N, k, BER_4FSK_awgn);
errors_4FSK_fading = simRS(bitrate, flightTime, N, k, BER_4FSK_fading);

figure(4)
plot(errors_2FSK_fading(:, 1))
hold on;
plot(errors_2FSK_fading(:, 2))
xlabel("time (unitless)")
ylabel("Bit errors")
title("Bit errors vs time")
legend("Raw Errors 2FSK Rician Fading", "Received Errors 2FSK Rician Fading")
figure(5)
plot(errors_4FSK_fading(:, 1))
hold on;
plot(errors_4FSK_fading(:, 2))
xlabel("time (unitless)")
ylabel("Bit errors")
title("Bit errors vs time")
legend("Raw Errors 4FSK Rician Fading", "Received Errors 4FSK Rician Fading")

