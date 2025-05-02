function [distance, elevation] = computeDistance(lat1, lon1, alt1, lat2, lon2, alt2)
    % Calculate straight line distance between 2 points in space that are
    % represented by their geodetic latitude, longitude and altitude.
    %
    % (lat1, lon1, alt1) point is the reference point.
    % lat1, lat2 - geodetic  latitude.
    % lon1, lon2 - geodetic longitude.
    % alt1, alt2 - altitude is in m.
    % distance is in km.
    % elevation is in degrees.

    %   Copyright 2018-2022 The MathWorks, Inc.
    
    % Convert the two geodetic points to ITRF
    [x1, y1, z1] = geodetic2ITRF(lat1, lon1, alt1);
    [x2, y2, z2] = geodetic2ITRF(lat2, lon2, alt2);
    
    % Find delta vector and convert to NED frame
    lat1rad = lat1 * pi / 180;
    lon1rad = lon1 * pi / 180;
    coslat = cos(lat1rad);
    sinlat = sin(lat1rad);
    coslon = cos(lon1rad);
    sinlon = sin(lon1rad);
    R = [-sinlat*coslon -sinlon -coslat*coslon; ...
        -sinlat*sinlon coslon -coslat*sinlon; ...
        coslat 0 -sinlat];

    deltaNED = R' * [x2-x1 y2-y1 z2-z1]';
    r = norm(deltaNED);
    distance = r * 1e-3; % convert to km
    elevation = asin(-deltaNED(3)/r) * 180 / pi;
end

function [x, y, z] = geodetic2ITRF(lat, lon, alt)
    % Convert a point in space specified by its geodetic location as (lat,
    % lon, alt) to ITRF centered at Earth's center. 
    % lat - geodetic latitude.
    % lon - geodetic longitude.
    % alt - geodetic altitude in m. 
    % x, y, z  - ITRF co-ordinates in m.
    
    latRad = lat * pi / 180;
    lonRad = lon * pi / 180;
    
    eqtRadius = 6378137; % Equatorial radius in m
    e2 = 0.0066943799; % eccentricity squared
    v = eqtRadius / sqrt( (1 - e2*sin(latRad)^2) );
    
    x = (v + alt) * cos(latRad) * cos(lonRad);
    y = (v + alt) * cos(latRad) * sin(lonRad);
    z = ((1-e2)*v + alt) * sin(latRad);
end
