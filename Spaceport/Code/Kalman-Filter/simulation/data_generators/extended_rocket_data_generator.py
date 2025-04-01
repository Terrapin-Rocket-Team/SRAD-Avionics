import numpy as np
import math
from data_generators.base_data_generator import BaseDataGenerator
from typing import Optional, Union, Callable
from atmospheric_model.atmosphere import AtmosphereModel

# Assumes flat Earth for now (later add rotation of Earth into dynamics)
class EnhancedRocketDataGenerator(BaseDataGenerator):
    """Enhanced rocket flight data generator with sophisticated drag modeling."""
    def __init__(self, 
                 rocket,
                 loop_frequency: float = 50,
                 pre_launch_delay: float = 0.0,
                 launch_angle: float = 0.0,
                 wind_affector: Optional[Union[np.ndarray, Callable[[float], np.ndarray]]] = None):
        """
        :param rocket: A rocket object describing the physical parameters.
        :param loop_frequency: Frequency at which data is generated (Hz).
        :param pre_launch_delay: How many seconds the rocket sits idle before launching.
        :param launch_angle: Launch angle relative to the vertical (degrees). Picks a random heading angle for the launch.
        :param wind_affector: Either a fixed 3D numpy array/list or a callable that takes time and returns a 3D numpy array
        """
        self.rocket = rocket
        self.loop_frequency = loop_frequency
        self.pre_launch_delay = pre_launch_delay
        self.launch_angle = np.deg2rad(launch_angle)
        # Add random heading angle between 0 and 2π
        self.heading_angle = np.random.uniform(0, 2 * np.pi)
        self.wind_affector = wind_affector
        self.atmosphere = AtmosphereModel()

    def euler_to_quaternion(self, roll, pitch, yaw):
        """
        Convert Euler angles to quaternion.
        
        :param roll: Roll angle in radians
        :param pitch: Pitch angle in radians
        :param yaw: Yaw angle in radians
        :return: Quaternion as [w, x, y, z]
        """
        cr = np.cos(roll * 0.5)
        sr = np.sin(roll * 0.5)
        cp = np.cos(pitch * 0.5)
        sp = np.sin(pitch * 0.5)
        cy = np.cos(yaw * 0.5)
        sy = np.sin(yaw * 0.5)
        
        w = cr * cp * cy + sr * sp * sy
        x = sr * cp * cy - cr * sp * sy
        y = cr * sp * cy + sr * cp * sy
        z = cr * cp * sy - sr * sp * cy
        
        return np.array([w, x, y, z])
    
    def quaternion_to_rotation_matrix(self, q):
        """
        Convert quaternion to rotation matrix.
        
        :param q: Quaternion as [w, x, y, z]
        :return: 3x3 rotation matrix
        """
        w, x, y, z = q
        
        # Normalize quaternion
        norm = np.sqrt(w*w + x*x + y*y + z*z)
        if norm > 0:
            w, x, y, z = w/norm, x/norm, y/norm, z/norm
        
        # Quaternion to rotation matrix
        xx, yy, zz = x*x, y*y, z*z
        xy, xz, yz = x*y, x*z, y*z
        wx, wy, wz = w*x, w*y, w*z
        
        R = np.array([
            [1 - 2*(yy + zz), 2*(xy - wz), 2*(xz + wy)],
            [2*(xy + wz), 1 - 2*(xx + zz), 2*(yz - wx)],
            [2*(xz - wy), 2*(yz + wx), 1 - 2*(xx + yy)]
        ])
        
        return R
    
    def quaternion_multiply(self, q1, q2):
        """
        Multiply two quaternions.
        
        :param q1: First quaternion as [w, x, y, z]
        :param q2: Second quaternion as [w, x, y, z]
        :return: Result quaternion as [w, x, y, z]
        """
        w1, x1, y1, z1 = q1
        w2, x2, y2, z2 = q2
        
        w = w1*w2 - x1*x2 - y1*y2 - z1*z2
        x = w1*x2 + x1*w2 + y1*z2 - z1*y2
        y = w1*y2 + y1*w2 + z1*x2 - x1*z2
        z = w1*z2 + z1*w2 + x1*y2 - y1*x2
        
        return np.array([w, x, y, z])
    
    def quaternion_conjugate(self, q):
        """
        Calculate the conjugate of a quaternion.
        
        :param q: Quaternion as [w, x, y, z]
        :return: Conjugate quaternion as [w, -x, -y, -z]
        """
        return np.array([q[0], -q[1], -q[2], -q[3]])
    
    def quaternion_normalize(self, q):
        """
        Normalize a quaternion.
        
        :param q: Quaternion as [w, x, y, z]
        :return: Normalized quaternion
        """
        norm = np.sqrt(np.sum(q**2))
        if norm > 0:
            return q / norm
        return q
    
    def angular_velocity_to_quaternion_derivative(self, q, omega):
        """
        Convert angular velocity to quaternion derivative.
        
        :param q: Current quaternion as [w, x, y, z]
        :param omega: Angular velocity vector [wx, wy, wz]
        :return: Quaternion derivative
        """
        # Create pure quaternion from angular velocity
        omega_quat = np.array([0, omega[0], omega[1], omega[2]])
        
        # Calculate quaternion derivative: q_dot = 0.5 * q * omega_quat
        q_dot = 0.5 * self.quaternion_multiply(q, omega_quat)
        
        return q_dot
    
    def update_quaternion(self, q, angular_velocity, dt):
        """
        Update quaternion based on angular velocity using first-order integration.
        
        :param q: Current quaternion as [w, x, y, z]
        :param angular_velocity: Angular velocity vector [wx, wy, wz]
        :param dt: Time step
        :return: Updated quaternion
        """
        q_dot = self.angular_velocity_to_quaternion_derivative(q, angular_velocity)
        q_new = q + q_dot * dt
        return self.quaternion_normalize(q_new)

    def thrust_force_inertial(self, time, mass, rotation_matrix):
        """
        Calculate the thrust force in the body frame (along the x-axis).
        """
        if time < self.rocket.burnTime + self.pre_launch_delay and time >= self.pre_launch_delay:
            F_thrust = self.rocket.motorAccel * mass
            F_thrust_body = np.array([0, 0, F_thrust])
            return rotation_matrix @ F_thrust_body
        return np.zeros((3,))
    
    def gravity_force_inertial(self, mass, altitude, gravity):
        """
        Calculate the gravitational force in the inertial frame.
        """
        F_gravity_inertial = np.array([0, 0, -mass * gravity])  # Inertial frame gravity
        return F_gravity_inertial

    def calculate_friction_coefficient(self, reynolds: float, mach: float) -> float:
        """Calculate skin friction coefficient"""
        if not self.rocket.surface_roughness:
            # Perfect finish (partially laminar)
            if reynolds < 1.0e4:
                cf = 1.33e-2
            elif reynolds < 5.39e5:
                cf = 1.328 / math.sqrt(reynolds)
            else:
                cf = 1.0 / pow(1.50 * math.log(reynolds) - 5.6, 2) - 1700/reynolds
        else:
            # Turbulent
            if reynolds < 1.0e4:
                cf = 1.48e-2
            else:
                cf = 1.0 / pow(1.50 * math.log(reynolds) - 5.6, 2)

        # Compressibility corrections
        if mach < 0.9:
            cf *= 1 - 0.1 * mach**2
        elif mach > 1.1:
            if not self.rocket.surface_roughness:
                cf *= 1.0 / pow(1 + 0.045 * mach**2, 0.25)
            else:
                cf *= 1.0 / pow(1 + 0.15 * mach**2, 0.58)
        else:
            # Linear interpolation between M=0.9 and M=1.1
            if not self.rocket.surface_roughness:
                c1 = 1 - 0.1 * 0.9**2
                c2 = 1.0 / pow(1 + 0.045 * 1.1**2, 0.25)
            else:
                c1 = 1 - 0.1 * 0.9**2
                c2 = 1.0 / pow(1 + 0.15 * 1.1**2, 0.58)
            cf *= c2 * (mach - 0.9) / 0.2 + c1 * (1.1 - mach) / 0.2
            
        return cf

    def calculate_stagnation_cd(self, mach: float) -> float:
        """Calculate stagnation pressure coefficient"""
        if mach <= 1:
            pressure = 1 + mach**2/4 + mach**4/40
        else:
            pressure = 1.84 - 0.76/mach**2 + 0.166/mach**4 + 0.035/mach**6
        return 0.85 * pressure

    def calculate_base_cd(self, mach: float) -> float:
        """Calculate base drag coefficient"""
        if mach <= 1:
            return 0.12 + 0.13 * mach**2
        else:
            return 0.25 / mach

    def calculate_roughness_correction(self, mach: float) -> float:
        """Calculate roughness effects"""
        if not self.rocket.surface_roughness:
            return 0
        
        if mach < 0.9:
            correction = 1 - 0.1 * mach**2
        elif mach > 1.1:
            correction = 1 / (1 + 0.18 * mach**2)
        else:
            c1 = 1 - 0.1 * 0.9**2
            c2 = 1 / (1 + 0.18 * 1.1**2)
            correction = c2 * (mach - 0.9) / 0.2 + c1 * (1.1 - mach) / 0.2
            
        return 0.032 * pow(self.rocket.surface_roughness/self.rocket.length, 0.2) * correction

    # Used OpenRocket's Coefficient of Drag Approach
    def total_drag_coefficient(self, reynolds, mach_number: float) -> float:
        """
        Calculate the total drag coefficient by summing wave drag, base drag, and friction drag.
        :param reynolds: Reynolds Number
        :param mach_number: Mach number (ratio of rocket velocity to speed of sound)
        """
        
        # Calculate friction coefficient
        cf = self.calculate_friction_coefficient(reynolds, mach_number)
        
        # Calculate roughness effects
        roughness_cd = self.calculate_roughness_correction(mach_number)
        
        # Use maximum of Cf and roughness-limited value
        friction_cd = max(cf, roughness_cd)
        
        # Calculate pressure drag
        stagnation_cd = self.calculate_stagnation_cd(mach_number)
        
        # Calculate base drag
        base_cd = self.calculate_base_cd(mach_number)
        
        total_cd = friction_cd + stagnation_cd + base_cd
        return total_cd

    def drag_force_inertial(self, conditions, relative_velocity, relative_velocity_mag):
        """
        Calculate the drag force in the body frame incorporating wind_velocity as the wind
        """
         # Calculate the relative velocity vector (rocket relative to air)
        
        if relative_velocity_mag == 0:
            return np.zeros(3)
    
        mach_number = relative_velocity_mag / conditions["speed_of_sound"]
        reynolds = conditions["reynolds_number"]
        C_d = self.total_drag_coefficient(reynolds, mach_number)
        density = conditions["density"]
    
        # Calculate pure aerodynamic drag (opposite to relative velocity direction)
        drag_force = -0.5 * C_d * density * self.rocket.topCrossSectionalArea * relative_velocity_mag**2 * (relative_velocity / relative_velocity_mag) # top dominates
        return drag_force

    # Moment of Inertia calculation
    def moment_of_inertia(self, mass):
        """
        Calculate the moment of inertia tensor for the rocket.
        Current implementation only returns a scalar value, should be a 3x3 tensor.
        """
        # Approximate as a cylinder
        radius = self.rocket.diameter / 2
        length = self.rocket.length
        
        # Moment of inertia tensor for a cylinder
        Ixx = (mass/12) * (3*radius**2 + length**2)  # About x-axis
        Iyy = Ixx  # Same as Ixx due to symmetry
        Izz = (mass/2) * radius**2  # About z-axis (long axis)
        
        # Return full inertia tensor
        return np.array([[Ixx, 0, 0],
                        [0, Iyy, 0],
                        [0, 0, Izz]])

    
    def calculate_thrust_moment(self, F_thrust, rotation_matrix):
        """
        Calculate the moment (torque) generated by thrust at the center of mass.
        :param F_thrust: Thrust force vector in the body frame (N)
        """
        r_thrust_body = np.array([0, 0, -(self.rocket.length - self.rocket.CoG)])  # Distance from the center of mass to nozzle along the body axis, assuming the nozzle is on the z-axis in body frame
        r_thrust = rotation_matrix @ r_thrust_body
        return np.cross(r_thrust, F_thrust)

    def calculate_aero_moment(self, F_aero, rotation_matrix):
        """
        Calculate the aerodynamic moment generated by forces at the center of pressure.
        :param F_aero: Aerodynamic force (Drag, Lift, etc.) vector (N)
        """
        r_aero = rotation_matrix @ np.array([0, 0, -(self.rocket.CoG - self.rocket.CoP)])
        return np.cross(r_aero, F_aero)

    def total_force_and_moment_body(self, time, altitude, conditions, relative_velocity, relative_velocity_mag, rotation_matrix):
        """
        Calculate the total force (sum of thrust, gravity, and drag) and moment in the body frame.
        """
        current_mass = self.rocket.get_current_mass(time - self.pre_launch_delay)
        # print(current_mass)
        F_thrust_i = self.thrust_force_inertial(time, current_mass, rotation_matrix)
        # print(f"Thrust {F_thrust_i}")
        gravity = conditions["gravity"]
        F_gravity_i = self.gravity_force_inertial(current_mass, altitude, gravity)
        # print(F_gravity_i)
        F_drag_i = self.drag_force_inertial(conditions, relative_velocity, relative_velocity_mag)
        # print(f"Drag {F_drag_i}")
        total_force = F_thrust_i + F_gravity_i + F_drag_i
        M_thrust = self.calculate_thrust_moment(F_thrust_i, rotation_matrix)
        M_aero = self.calculate_aero_moment(F_drag_i, rotation_matrix)
        M_total = M_thrust + M_aero
        return total_force, M_total

    # Angular dynamics
    def angular_acceleration(self, torque, inertia):
        """
        Calculate angular acceleration from the torque and the moment of inertia.
        """
        return np.linalg.solve(inertia, torque)

    # If necessary can add information necessary to implement a magnetometer
    def generate(self) -> dict:
        """Generate rocket flight data."""
        dt = 1.0 / self.loop_frequency
        
        # Initialize data arrays
        time_list = []
        position = np.zeros((3,))
        velocity = np.zeros((3,))
        acceleration = np.array([0, 0, -9.81])
       
        roll, pitch, yaw = 0, self.launch_angle, self.heading_angle
        # Initialize orientation with quaternions
        # Convert initial roll, pitch, yaw to quaternion
        quaternion = self.euler_to_quaternion(roll, pitch, yaw)
        rotation_matrix = self.quaternion_to_rotation_matrix(quaternion)
        angular_velocity = np.zeros((3,))
        angular_acceleration = np.zeros((3,))

        # Data storage
        positions = []
        velocities = []
        accelerations = []
        quaternions = []
        angular_velocities = []
        angular_accelerations = []
        
        # Pre-launch phase
        pre_launch_steps = int(self.pre_launch_delay / dt)
        for i in range(pre_launch_steps):
            t_val = i * dt
            time_list.append(t_val)
            positions.append(position.copy())
            velocities.append(velocity.copy())
            accelerations.append(acceleration.copy())
            quaternions.append(quaternion.copy())
            angular_velocities.append(angular_velocity.copy())
            angular_accelerations.append(angular_acceleration.copy())
        
        # Flight phase
        i = pre_launch_steps
        while position[2] >= 0 or i == pre_launch_steps:
            t_val = i * dt
            time_list.append(t_val)

            # Calculate forces and moments at the current time
            altitude = position[2]  # Assuming altitude is along the z-axis
            relative_velocity = velocity - self.wind_affector(t_val)
            relative_velocity_mag = np.linalg.norm(relative_velocity)

            conditions = self.atmosphere.get_conditions(
                altitude=altitude,
                velocity=relative_velocity_mag,  # Using relative velocity magnitude
                characteristic_length=self.rocket.length
            )

            total_force, total_moment = self.total_force_and_moment_body(
                t_val, altitude, conditions, relative_velocity, relative_velocity_mag, rotation_matrix
            )
            
            # Translational motion
            # Update acceleration based on the total force and mass
            current_mass = self.rocket.get_current_mass(t_val)
            acceleration = total_force / current_mass
            velocity += acceleration * dt
            position += velocity * dt + .5 * acceleration * dt**2
            # print(position[2])

            # Update angular dynamics (orientation, angular velocity, angular acceleration)
            moment_of_inertia = self.moment_of_inertia(current_mass)
            angular_accel = self.angular_acceleration(total_moment, moment_of_inertia)
            angular_velocity += angular_accel * dt
            # Update quaternion using angular velocity
            quaternion = self.update_quaternion(quaternion, angular_velocity, dt)
            # Get rotation matrix from quaternion for force calculations
            rotation_matrix = self.quaternion_to_rotation_matrix(quaternion)
        
            # Store the current state for later retrieval
            positions.append(position.copy())
            velocities.append(velocity.copy())
            accelerations.append(acceleration.copy())
            # print(acceleration)
            quaternions.append(quaternion.copy())
            if len(quaternion) != 4:
                print("Error")
            angular_velocities.append(angular_velocity.copy())
            angular_accelerations.append(angular_accel.copy())

            # Increment time step
            i += 1

        # Convert to numpy arrays
        positions = np.array(positions)
        velocities = np.array(velocities)
        accelerations = np.array(accelerations)
        quaternions = np.array(quaternions)
        angular_velocities = np.array(angular_velocities)
        angular_accelerations = np.array(angular_accelerations)

        return {
            "time": np.array(time_list),
            "r_x": positions[:, 0],
            "r_y": positions[:, 1],
            "r_z": positions[:, 2],
            "v_x": velocities[:, 0],
            "v_y": velocities[:, 1],
            "v_z": velocities[:, 2],
            "a_x": accelerations[:, 0],
            "a_y": accelerations[:, 1],
            "a_z": accelerations[:, 2],
            "quaternion": quaternions,  
            "angular_vx": angular_velocities[:, 0],
            "angular_vy": angular_velocities[:, 1],
            "angular_vz": angular_velocities[:, 2],
            "angular_accel_x": angular_accelerations[:, 0],
            "angular_accel_y": angular_accelerations[:, 1],
            "angular_accel_z": angular_accelerations[:, 2],
        }