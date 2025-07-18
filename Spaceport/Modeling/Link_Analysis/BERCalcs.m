close all;

EbNo = 0:0.1:20;
BERpsk2 = berawgn(EbNo, "psk", 2, "diff");
BERpsk2n = berawgn(EbNo, "psk", 2, "nondiff");
BERpsk4 = berawgn(EbNo, "psk", 4, "diff");
BERpsk4n = berawgn(EbNo, "psk", 4, "nondiff");

BERfsk2 = berawgn(EbNo, "fsk", 2, "coherent");
BERfsk2n = berawgn(EbNo, "fsk", 2, "noncoherent");
BERfsk4 = berawgn(EbNo, "fsk", 4, "coherent");
BERfsk4n = berawgn(EbNo, "fsk", 4, "noncoherent");

BERmsk2 = berawgn(EbNo, "msk", "off");

BERcpfsk2 = berawgn(EbNo, "cpfsk", 2, 1, 1);
BERcpfsk4 = berawgn(EbNo, "cpfsk", 4, 1, 1);
BERcpmsk2 = berawgn(EbNo, "cpfsk", 2, 0.5, 1);
BERcpmsk4 = berawgn(EbNo, "cpfsk", 4, 0.5, 1);

%figure(1)
%semilogy(EbNo, BERpsk2)
%hold on;
%semilogy(EbNo, BERpsk2n)

%legend("Differential 2-PSK", "Non-Differential 2-PSK")
%axis([0, 20, 10^-10, 10^0])

%figure(2)
%semilogy(EbNo, BERpsk4)
%hold on;
%semilogy(EbNo, BERpsk4n)

%legend("Differential 4-PSK", "Non-Differential 4-PSK")
%axis([0, 20, 10^-10, 10^0])

figure(3)
semilogy(EbNo, BERfsk2)
hold on;
semilogy(EbNo, BERfsk2n)

legend("Coherent 2-FSK", "Non-Coherent 2-FSK")
axis([0, 20, 10^-10, 10^0])
ylabel("BER")
xlabel("Eb/No (dB)")

figure(4)
semilogy(EbNo, BERfsk4)
hold on;
semilogy(EbNo, BERfsk4n)

legend("Coherent 4-FSK", "Non-Coherent 4-FSK")
axis([0, 20, 10^-10, 10^0])
ylabel("BER")
xlabel("Eb/No (dB)")

figure(5)
semilogy(EbNo, BERcpfsk2)
hold on;
semilogy(EbNo, BERcpfsk4)

legend("2-CPFSK", "4-CPFSK")
axis([0, 20, 10^-10, 10^0])
ylabel("BER")
xlabel("Eb/No (dB)")

figure(6)
semilogy(EbNo, BERcpmsk2)
hold on;
semilogy(EbNo, BERcpmsk4)

legend("2-CPMSK", "4-CPMSK")
axis([0, 20, 10^-10, 10^0])
ylabel("BER")
xlabel("Eb/No (dB)")

figure(7)
semilogy(EbNo, BERmsk2)

legend("2-MSK coherent")
axis([0, 20, 10^-10, 10^0])
ylabel("BER")
xlabel("Eb/No (dB)")


