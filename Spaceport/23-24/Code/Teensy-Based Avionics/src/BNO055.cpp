


double BNO055::get_pressure() {
    pressure = bmp.readPressure() / 100.0; // hPa
    return pressure;
}

double BNO055::get_temp() {
    temp = bmp.readTemperature(); // C
    return temp;
}

double BNO055::get_temp_f() {
    return (get_temp() * 9.0 / 5.0) + 32.0;
}
