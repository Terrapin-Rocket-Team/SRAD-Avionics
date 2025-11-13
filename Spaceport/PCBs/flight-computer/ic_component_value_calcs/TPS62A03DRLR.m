%%
clc; close all; clear

V_OUT = 4.5; % 3.3V + V_DROPOUT (= 1.2V)
R2 = 100e3;
V_FB = .6;

R1 = R2 * ((V_OUT / V_FB) - 1)

%% R1 Actual
% R1 = 649e3;
% V_OUT = V_FB * ((R1 + R2) / R2)