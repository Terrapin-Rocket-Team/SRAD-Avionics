function loss = polarizationLoss(angle, antenna_type_1, antenna_type_2)

if (antenna_type_1 == "linear") && (antenna_type_2 == "linear")
    loss = 20 * log10(abs(cosd(angle)));
    return
end
if ((antenna_type_1 == "linear") && (antenna_type_2 == "RHCP" || antenna_type_2 == "LHCP")) || (antenna_type_2 == "linear") && (antenna_type_1 == "RHCP" || antenna_type_1 == "LHCP")
    loss = -3;
    return
end
if (antenna_type_1 == "RHCP" || antenna_type_1 == "LHCP") && antenna_type_1 == antenna_type_2
    loss = 0;
    return
end
if (antenna_type_1 == "RHCP" || antenna_type_1 == "LHCP") && antenna_type_1 ~= antenna_type_2
    loss = -30;
    return
end

end