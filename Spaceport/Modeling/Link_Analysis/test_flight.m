close all;

TX_ant_eff = 0.40; % percent
TX_ant_gain = 0; % dBi
TX_ant_type = "linear"; % linear, RHCP, or LHCP
TX_pow = 30; % dBm
RX_ant_eff = 0.60; % percent
RX_ant_gain = 5; % dBi
RX_ant_type = "linear"; % linear, RHCP, or LHCP

altitude = 0:0.5:2755;
lambda = physconst('LightSpeed')/433e6;
L = fspl(altitude+100, lambda);

Pr = TX_pow + 10*log(TX_ant_eff) + TX_ant_gain + RX_ant_gain - L + 10*log(RX_ant_eff);

hold on;
plot(altitude, Pr)
plot((0:(172-156))/(172-156)*2755,rssi(156:172))
xlabel("altitude (ft)")
ylabel("Received signal (dBm)")