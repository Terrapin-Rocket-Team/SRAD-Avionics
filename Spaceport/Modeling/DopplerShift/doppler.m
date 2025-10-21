%%
close all; clear; clc

% Load open rocket sim
data = load('comp_rocket_openRocket_velocity_profile.mat');
v = data.comp_rocket_openRocket_velocity_profile.x_VerticalVelocity_m_s_;

% figure()
% plot(v);
% xlabel('Time (s)');
% ylabel('Vertical Velocity (m/s)');
% title('Vertical Velocity Profile of the Rocket');
% grid on;

%%
close all; clc

freq = 1.2e9;
lambda = 3*10^8 / freq;

doppler_shift = v(1:1000) ./ lambda;
doppler_rate = movmean(diff(doppler_shift), 25);

% Figure Code
font_size = 15;
axSize = 13;

figure()
subplot(1,2,1)
plot(doppler_shift, LineWidth=2)
xlabel('Time (s)', FontSize=font_size)
ylabel('Doppler Shift (Hz)', FontSize=font_size)
title('Estimated Doppler Shift at 23cm (Hz)', FontSize=font_size)
ax = gca;
ax.FontSize = axSize;  % Font Size of 15

[max_val, idx] = max(doppler_shift);
label_str = sprintf('Max: %.2f Hz', max_val);
text(idx, max_val, label_str, ...
    'VerticalAlignment', 'bottom', ...
    'HorizontalAlignment', 'left', ...
    'FontSize', axSize-2);

% figure()
subplot(1,2,2)
plot(doppler_rate, LineWidth=2)
xlabel('Time (s)', FontSize=font_size)
ylabel('Doppler  Rate (Hz/s)', FontSize=font_size)
title('Estimated Doppler Shift Rate at 23cm (Hz/s)', FontSize=font_size)
ax = gca;
ax.FontSize = axSize;  % Font Size of 15

[max_val, idx] = max(doppler_rate);
label_str = sprintf('Max: %.2f Hz/s', max_val);
text(idx, max_val, label_str, ...
    'VerticalAlignment', 'bottom', ...
    'HorizontalAlignment', 'left', ...
    'FontSize', axSize-2);
grid on;  % Add grid to the Doppler rate plot
