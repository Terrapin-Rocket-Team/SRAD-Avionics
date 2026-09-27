%%
close all; clear; clc

I = 67.5e-6;
R2 = 15e3;
VR = 1.25;
VO = 3.3;

R1 = ((((VO - (I * R2)) / VR) - 1) / R2)^-1;

% Confirm VOUT = 3.3V
VR * (1 + (R2 / R1)) + (I * R2)

disp(R1)
