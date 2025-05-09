close all;

fd = table2array(struct2table(load('SACFlightData.mat')));
alt = table2array(fd(:,1));

figure()
hold on;
yyaxis left;
colororder("default")
plot(0:0.1:(length(Pr)-1)/10, Pr + 97, "-")
plot(0:0.1:(length(Pr)-1)/10, Pr + 88, "-")
plot(0:0.1:(length(Pr)-1)/10, Pr + 109, "-")
xlabel("Time (s)")
ylabel("Power (dBm)")
yyaxis right;
colororder("black")
plot(0:0.1:(length(Pr)-1)/10, alt, "--")
ylabel("Altitude (ft)")
title("Link margin (sensitivity)")
legend("Link margin 500 kbps", "Link margin 1 Mbps", "Link margin 9.6 kbps (telemetry)", "Altitude")
ax = gca;
ax.YAxis(1).Color = 'k';
ax.YAxis(2).Color = 'k';
grid on
fontsize(scale=1.4)

figure()
semilogy(0:0.1:(length(Pr)-1)/10, BER_2FSK_fading)
hold on;
semilogy(0:0.1:(length(Pr)-1)/10, BER_4FSK_fading)
xlabel("Time (s)")
ylabel("BER (errors/total)")
title("BER vs time")
legend("Estimated Rician Fading BER 2FSK", "Estimated Rician Fading BER 4FSK")
axis([0, (length(Pr)-1)/10, 10^-8, 10^0])
grid on
fontsize(scale=1.4)

figure()
plot(errors_2FSK_fading(:, 1), errors_2FSK_fading(:, 4))
hold on;
plot(errors_2FSK_fading(:, 1), errors_4FSK_fading(:, 4))
xlabel("Time (s)")
ylabel("Symbol errors")
title("Symbol errors vs time")
legend("Symbol Errors 2FSK Rician Fading", "Symbol Errors 4FSK Rician Fading")
grid on
fontsize(scale=1.4)
