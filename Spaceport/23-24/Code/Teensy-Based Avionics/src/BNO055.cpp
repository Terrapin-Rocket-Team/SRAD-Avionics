





virtual imu::Quaternion get_orientation() {
return getQuat();
}

virtual imu::Vector<3> get_acceleration() {
return getVector(VECTOR_ACCELEROMETER);
}

virtual imu::Vector<3> get_orientation_euler() {
return getVector(EULER);
}


