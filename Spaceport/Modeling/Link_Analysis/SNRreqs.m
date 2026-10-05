close all;

selectedEbNo = 10:.1:20;

SNRdr40kbw80k = selectedEbNo + 10*log10(40e3/80e3); % 2-FSK
SNRdr100kbw200k = selectedEbNo + 10*log10(40e3/80e3); % 2-FSK
SNRdr500kbw1M = selectedEbNo + 10*log10(500e3/1e6); % 2-FSK
SNRdr40kbw60k = selectedEbNo + 10*log10(40e3/60e3); % 4-FSK
SNRdr500kbw750k = selectedEbNo + 10*log10(500e3/750e3); % 4-FSK
SNRdr1Mbw1_5M = selectedEbNo + 10*log10(1e6/1.5e6); % 4-FSK

figure(1);
hold on;
plot(selectedEbNo, SNRdr40kbw80k);
plot(selectedEbNo, SNRdr100kbw200k);
plot(selectedEbNo, SNRdr500kbw1M);

legend("40kbps 80khz", "100kbps 200khz", "500kbps 1Mhz");
ylabel("SNR required (dB)")
xlabel("Eb/No (dB)")

figure(2);
hold on;
plot(selectedEbNo, SNRdr40kbw60k);
plot(selectedEbNo, SNRdr500kbw750k);
plot(selectedEbNo, SNRdr1Mbw1_5M);

legend("40kbps 60khz", "500kbps 750khz", "1Mbps 1.5Mhz");
ylabel("SNR required (dB)")
xlabel("Eb/No (dB)")
