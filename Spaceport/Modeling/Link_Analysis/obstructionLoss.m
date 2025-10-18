% Obstruction loss calcs
% source: https://courses.grainger.illinois.edu/ece458/ITU-R-REC-P.452-15-201309.pdf
function loss = obstructionLoss(f, h_1, h_2, d)
% f in Hz, h_1, h_2 in ft, d in miles
a_e = 8500; % km, standard value
h_1 = h_1*0.3048; % km (converted from ft)
h_2 = h_2*0.3048; % km (converted from ft)
d = d*(1/0.6213712); % km (converted from mi)

f = f*10^-9; % GHz
c = physconst('LightSpeed');
lambda = c/(f*10^9); % m

Beta = 1; % assume high frequency (>300MHz)

d_LOS = sqrt(2*a_e)*(sqrt(h_1)+sqrt(h_2))*0.6213712; % convert km to mi
a_em = 500*(d/(sqrt(h_1)+sqrt(h_2)))^2;

%ep_r = 22;
%sig = 5; % S/m
%K_H = 0.036*(a_em*f)^(-1/3)*((ep_r-1)^2 + (18*sig/f)^2)^(-1/4);
%K_V = K_H*(ep_r^2 + (18*sig/f)^2)^(1/2);
%k = 0.5*K_H + 0.5*K_V;

X = 21.88*Beta*(f/(a_em^2))^(1/3)*d;
Y1 = 0.9575*Beta*((f^2)/a_em)^(1/3)*h_1;
Y2 = 0.9575*Beta*((f^2)/a_em)^(1/3)*h_2;

F = 0;

if (X >= 1.6)
    F = 11 + 10*log10(X) - 17.6*X;
end
if (X < 1.6)
    F = -20*log10(X) - 5.6488*X^1.425;
end

B_1 = Beta*Y1;
B_2 = Beta*Y2;

if (B_1 > 2)
    G_1 = 17.6*(B_1 - 1.1)^0.5 - 5*log10(B_1 - 1.1) - 8;
end
if (B_1 <= 2)
    G_1 = 20*log10(B_1 + 0.1*B_1^3);
end

if (B_2 > 2)
    G_2 = 17.6*(B_2 - 1.1)^0.5 - 5*log10(B_2 - 1.1) - 8;
end
if (B_2 <= 2)
    G_2 = 20*log10(B_2 + 0.1*B_2^3);
end

K = 0.01; % set k to reasonable small value
G_1 = max(G_1, 2+20*log(K));
G_2 = max(G_2, 2+20*log(K));

Ah = -F - G_1 - G_2;

c = (h_1-h_2)/(h_1+h_2);
m = 250*d^2/(a_e*(h_1+h_2));

b = 2*sqrt((m+1)/(3*m))*cos(pi/3+(1/3)*acos(3*c*sqrt((3*m)/((m+1)^3))/2));

d1 = d/2*(1+b);
d2 = d - d1;

h = ((h_1 - 500*d1^2/a_e)*d2 + (h_2  - 500*d2^2/a_e)*d1)/d;

hreq = 17.456*sqrt(d1*d2*lambda/d);

if (Ah > 0 && h < hreq && d < d_LOS)
    loss = (1-h/hreq)*Ah;
else
    loss = 0;
end
loss = -loss;

end