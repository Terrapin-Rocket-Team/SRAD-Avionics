format long
data = readtable('data/L1_Drift_Control_Test_2-3-24.csv');


offset = 49961;

columnIndex = 1; % Change to the index of the column you want to check

% Find rows containing NaN values in the specific column
rowsWithNaN = isnan(data{:, columnIndex});

% Remove rows containing NaN values in the specific column from the table
data = data(~rowsWithNaN, :);

columnIndex = 43; % Change to the index of the column you want to check

% Find rows containing NaN values in the specific column
rowsWithNaN = isnan(data{:, columnIndex});

% Remove rows containing NaN values in the specific column from the table
data = data(~rowsWithNaN, :);

alt = data.BarAltitude;
time = data.Time_ms_;
% Calculate running average with window size of 21 (10 before, 10 after, including the value itself)
runningAvg = movmean(alt, [50, 50], 'Endpoints', 'shrink');

% Calculate the standard deviation of the original barometric altitude data
stdDev = movstd(alt, [50, 50], 'Endpoints', 'shrink');

% Find indices where the barometric altitude deviates more than 5 standard deviations from the running average
% This comparison is made against the runningAvg for a fair comparison
outlierIndices = abs(alt - runningAvg) > 3 * stdDev;

% Remove the outlier rows from the data
cleanedData = data(~outlierIndices, :);
alt = cleanedData.BarAltitude;
time = cleanedData.Time_ms_;
lon = cleanedData.GPSLongitude/10^7;
lat = cleanedData.GPSLatitude/10^7;
accel_x = cleanedData.IMUAccelX;
accel_y = cleanedData.IMUAccelY;
accel_z = cleanedData.IMUAccelZ;

num_points = length(lon);
x_dist = zeros(num_points, 1);
y_dist = zeros(num_points, 1);

std_error = 10.5;
disp(lon(1))
for i = 2:num_points
    x_dist(i) = haversine(lon(1), lon(i));
    y_dist(i) = haversine(lat(1), lat(i));
end

initial_control = [0; 0; 0];
initial_state = [0; 0; 0; 0; 0; 0];
P =                   [1 0 0 1 0 0;
                      0 1 0 0 1 0;
                      0 0 1 0 0 1;
                      1 0 0 1 0 0;
                      0 1 0 0 1 0;
                      0 0 1 0 0 1];
P = 500*P;

x_pos = [];
y_pos = [];
z_pos = [];
x_velo = [];
y_velo = [];
z_velo = [];
x_pos_measured = [];
y_pos_measured = [];
z_pos_measured = [];

kf = KalmanFilter(initial_control, initial_state, P);
kf = kf.calculateInitialValues(0.1);

initial_lon = lon(1);
initial_lat = lat(1);

for i = 2:size(time)
    x = haversine(initial_lon, lon(i)) + randn(1)*std_error;
    y = haversine(initial_lat, lat(i)) + randn(1)*std_error;
    z = alt(i) + randn(1)*std_error;

    measurement = [x; y; z];
    control = [accel_x(i); accel_y(i); accel_z(i) - 9.8];
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
subplot(3, 1, 3)
plot(time, alt, time, [0, z_pos]);
xlabel('Time (s)');
ylabel('Z-position (m)');
legend('True', 'Estimated');

subplot(3, 1, 1)
plot(time, x_dist, time, [0, x_pos]');
xlabel('Time (s)');
ylabel('X-position (m)');
legend('Measured', 'Estimated');

subplot(3, 1, 2)
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