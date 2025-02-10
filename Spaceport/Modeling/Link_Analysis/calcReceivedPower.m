function Pr = calcReceivedPower(f, GS_lat, GS_long, GS_alt, TX_ant_eff, TX_ant_gain, TX_ant_type, TX_pow, ...
    RX_ant_eff, RX_ant_gain, RX_ant_type, alt_scale_factor, flightDataFile, orientDataFile)

flightData = table2array(struct2table(load(flightDataFile + '.mat')));
orientData = table2array(struct2table(load(orientDataFile + '.mat')));

ftTom = 0.3048;

% load rocket alt
% no idea why these need to be converted to arrays twice
rocket_alt = table2array(flightData(:,1).*ftTom.*alt_scale_factor); % easy way to get a dataset for arbitrary height
rocket_lat = table2array(flightData(:,2));
rocket_long = table2array(flightData(:,3));
rocket_quat_w = table2array(orientData(:,1));
rocket_quat_x = table2array(orientData(:,2));
rocket_quat_y = table2array(orientData(:,3));
rocket_quat_z = table2array(orientData(:,4));

lambda = physconst('LightSpeed')/f;

L = zeros(size(flightData,1), 1);
R = zeros(size(flightData,1), 1);
ant_angles = zeros(size(flightData,1), 1);

% create GRS-80 spheroid
s = oblateSpheroid;
s.SemimajorAxis = 6378137;
s.InverseFlattening = 298.257222101;

Ant_plane_norm = [-0.1 0 -1]; % axis of antenna
Ant_norm = quatrotate([rocket_quat_w, rocket_quat_x, rocket_quat_y, rocket_quat_z], Ant_plane_norm);

for k = 1:size(flightData,1)

    R(k) = computeDistance(GS_lat, GS_long, GS_alt, rocket_lat(k,1), rocket_long(k,1), abs(rocket_alt(k,1)))*1e3;
    L(k) = fspl(R(k),lambda);
    
    [xR,yR,zR] = geodetic2ned(GS_lat, GS_long, GS_alt, rocket_lat(k,1), rocket_long(k,1), rocket_alt(k,1), s);
    GS_norm = cross([yR,-xR,0], [xR,yR,zR]);
    ant_angles(k) = acosd(dot(GS_norm, Ant_norm(k,:))/(norm(GS_norm)*norm(Ant_norm(k,:))));
end

% polarization loss
polloss_guess = polarizationLoss(ant_angles, TX_ant_type, RX_ant_type);

% final result
Pr = TX_pow + 10*log(TX_ant_eff) + TX_ant_gain + RX_ant_gain - L + 10*log(RX_ant_eff) + polloss_guess;

end