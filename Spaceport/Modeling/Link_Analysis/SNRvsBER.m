close all;

SNR = 0:0.5:20;
% both 2-FSK and 4-FSK data rate and frequency deviation are chosen for
% modulation index = 1
EbN0dr40kbw80k = SNR - 10*log10(40e3/80e3); % 2-FSK
EbN0dr100kbw200k = SNR - 10*log10(40e3/80e3); % 2-FSK
EbN0dr500kbw1M = SNR - 10*log10(500e3/1e6); % 2-FSK
EbN0dr40kbw60k = SNR - 10*log10(40e3/60e3); % 4-FSK
EbN0dr500kbw750k = SNR - 10*log10(500e3/750e3); % 4-FSK
EbN0dr1Mbw1_5M = SNR - 10*log10(1e6/1.5e6); % 4-FSK

ENs = [EbN0dr40kbw80k; EbN0dr100kbw200k; EbN0dr500kbw1M; EbN0dr40kbw60k; EbN0dr500kbw750k; EbN0dr1Mbw1_5M];
results = [];
titles = ["40kbps 80khz", "100kbps 200khz", "500kbps 1Mhz", "40kbps 60kz", "500kbps 750khz", "1Mbps 1.5Mhz"];

for k=1:6
    EbNo = ENs(k,:);
    if (k <= 3)
    results(k,1,:) = berawgn(EbNo, "fsk", 2, "noncoherent");
    results(k,2,:) = berawgn(EbNo, "cpfsk", 2, 1, 1);
    end
    if (k > 3)
    results(k,1,:) = berawgn(EbNo, "fsk", 4, "noncoherent");
    results(k,2,:) = berawgn(EbNo, "cpfsk", 4, 1, 1);
    end

    figure(k)
    semilogy(SNR, squeeze(results(k,1,:)))
    hold on;
    semilogy(SNR, squeeze(results(k,2,:)))

    title(titles(k))
    if (k <= 3)
    legend("2-FSK", "2-CPFSK")
    end
    if (k > 3)
    legend("4-FSK", "4-CPFSK");
    end
    axis([0, 20, 10^-10, 10^0]);

end

