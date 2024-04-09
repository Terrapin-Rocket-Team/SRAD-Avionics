clear
clearvars
clearvars -global
format long
data = readtable('2023-01-07-TeleMega.csv');

lon = data.longitude;
lat = data.latitude;
alt = data.height;
accel_x = data.accel_z;
accel_y = data.accel_y;
accel_z = data.accel_x;
barometer_alt = data.altitude;
time = data.time;

num_points = length(lon);
x_dist = zeros(num_points, 1);
y_dist = zeros(num_points, 1);

std_error = 10.5;
for i = 2:num_points
    x_dist(i) = haversine(lon(1), lon(i));
    y_dist(i) = haversine(lat(1), lat(i));
end

initial_control = zeros(1,3);
initial_state = zeros(1,6);
P = eye(6)*500;
VarMat= [1,1,1,0.01,0.01,0.01,1];

x_pos = [];
y_pos = [];
z_pos = [];
x_velo = [];
y_velo = [];
z_velo = [];
x_pos_measured = [];
y_pos_measured = [];
z_pos_measured = [];
x_accel_measured = [];
y_accel_measured = [];
z_accel_measured = [];

kf = MultiVarKalmanFilter6(initial_control, initial_state, P);
kf = kf.calculateInitialValues(0.1);

initial_lon = lon(1);
initial_lat = lat(1);

for i = 2:size(time)
    x = haversine(initial_lon, lon(i)) + randn(1)*std_error
    y = haversine(initial_lat, lat(i)) + randn(1)*std_error
    z = alt(i) + randn(1)*std_error

    measurement = [x; y; z; z];
    control = [accel_x(i); accel_y(i); accel_z(i)];
    kf = kf.iterate(time(i) - time(i - 1), measurement, control);
    x_pos = [x_pos, kf.X(1)];
    y_pos = [y_pos, kf.X(2)];
    z_pos = [z_pos, kf.X(3)];
    x_velo = [x_velo, kf.X(4)];
    y_velo = [y_velo, kf.X(5)];
    z_velo = [z_velo, kf.X(6)];
    x_pos_measured = [x_pos_measured, x];
    y_pos_measured = [y_pos_measured, y];
    z_pos_measured = [z_pos_measured, z];
end

final_state = kf.X;
figure(1)
subplot(6, 1, 3)
plot(time, alt, time, [0, z_pos_measured]', time, [0, z_pos]');
xlabel('Time (s)');
ylabel('Z-position (m)');
legend('True', 'Measured', 'Estimated');

subplot(6, 1, 1)
plot(time, x_dist, time, [0, x_pos]');
xlabel('Time (s)');
ylabel('X-position (m)');
legend('Measured', 'Estimated');

subplot(6, 1, 2)
plot(time, y_dist, time, [0, y_pos]');
xlabel('Time (s)');
ylabel('Y-position (m)');
legend('Measured', 'Estimated');


function distance = haversine(lon1, lon2)
    % Radius of the Earth in meters
    R = 6371000; % Approximately 6,371,000 meters
    
    % Convert degrees to radians
    lon1 = deg2rad(lon1);
    lon2 = deg2rad(lon2);

    % Haversine formula
    dlon = lon2 - lon1;
    h = sin(dlon/2)^2;
    c = 2 * atan2(sqrt(h), sqrt(1-h));
    distance = R * c; % Distance in meters
end