import serial as pyserial
import serial.tools.list_ports
import threading
import queue
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Serial manager to open/close port and read data
class SerialManager:
    def __init__(self):
        self.ser = None
        self.thread = None
        self.running = False
        self.quat_queue = queue.Queue()
        self.accel_queue = queue.Queue()

    def list_ports(self):
        return [p.device for p in pyserial.tools.list_ports.comports()]

    def open(self, port, baud=115200):
        if self.ser and self.ser.is_open:
            print("Port already open")
            return
        try:
            self.ser = pyserial.Serial(port, baud, timeout=1)
        except Exception as e:
            print(f"Failed to open port {port}: {e}")
            return
        self.running = True
        self.thread = threading.Thread(target=self._read_loop, daemon=True)
        self.thread.start()
        print(f"Opened port {port} at {baud} baud")

    def close(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1)
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Serial port closed")

    def _read_loop(self):
        while self.running and self.ser and self.ser.is_open:
            try:
                line = self.ser.readline().decode('ascii', errors='replace').strip()
            except Exception:
                continue
            # Parse quaternion lines
            if line.startswith('Q,'):
                parts = line[2:].split(',')
                if len(parts) == 4:
                    try:
                        q = np.array([float(p) for p in parts])
                        self.quat_queue.put(q)
                    except ValueError:
                        pass
            # Parse acceleration lines
            elif line.startswith('A,'):
                parts = line[2:].split(',')
                if len(parts) == 3:
                    try:
                        a = np.array([float(p) for p in parts])
                        self.accel_queue.put(a)
                    except ValueError:
                        pass
            else:
                print("Received:", line)

    def get_latest_quaternion(self):
        q = None
        while not self.quat_queue.empty():
            q = self.quat_queue.get()
        return q

    def get_latest_accel(self):
        a = None
        while not self.accel_queue.empty():
            a = self.accel_queue.get()
        return a

# Quaternion helper
def quaternion_conjugate(q):
    w, x, y, z = q
    return np.array([w, -x, -y, -z])

def quat_mult(a, b):
    w0, x0, y0, z0 = a
    w1, x1, y1, z1 = b
    return np.array([
        w0*w1 - x0*x1 - y0*y1 - z0*z1,
        w0*x1 + x0*w1 + y0*z1 - z0*y1,
        w0*y1 - x0*z1 + y0*w1 + z0*x1,
        w0*z1 + x0*y1 - y0*x1 + z0*w1
    ])

def rotate_vector(v, q):
    # Rotate vector v (3,) by quaternion q [w,x,y,z]
    vq = np.concatenate(([0.0], v))
    q_conj = quaternion_conjugate(q)
    tmp = quat_mult(q, vq)
    res = quat_mult(tmp, q_conj)
    return res[1:]

# Interactive script
if __name__ == "__main__":
    sm = SerialManager()
    print("Available ports:", sm.list_ports())
    print("Commands:")
    print("  list               - show available serial ports")
    print("  open <port> [baud] - open serial port")
    print("  close              - close current port")
    print("  exit               - quit")
    
    # Start 3D plot
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.set_xlim([-1,1]); ax.set_ylim([-1,1]); ax.set_zlim([-1,1])
    ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_zlabel('Z')

    def update(frame):
        q = sm.get_latest_quaternion()
        a_raw = sm.get_latest_accel()
        if q is not None:
            # body axes
            x_v = rotate_vector([1,0,0], q)
            y_v = rotate_vector([0,1,0], q)
            z_v = rotate_vector([0,0,1], q)
            # earth-frame acceleration direction
            acc_dir = a_raw if a_raw is not None else None
            ax.cla()
            ax.set_xlim([-1,1]); ax.set_ylim([-1,1]); ax.set_zlim([-1,1])
            ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_zlabel('Z')
            # plot axes
            ax.quiver(0,0,0, *x_v, color='r', length=1)
            ax.quiver(0,0,0, *y_v, color='g', length=1)
            ax.quiver(0,0,0, *z_v, color='b', length=1)
            # plot acceleration vector if available (normalized)
            if acc_dir is not None:
                norm = np.linalg.norm(acc_dir)
                if norm>1e-3:
                    acc_unit = acc_dir / norm
                    ax.quiver(0,0,0, *acc_unit, color='m', length=1, linewidth=2)
        return []

    ani = FuncAnimation(fig, update, interval=50)

    # Console thread
    def console_loop():
        while True:
            cmd = input("> ").split()
            if not cmd:
                continue
            if cmd[0] == "list":
                print("Available ports:", sm.list_ports())
            elif cmd[0] == "open" and len(cmd)>=2:
                port = cmd[1]
                baud = int(cmd[2]) if len(cmd)>=3 else 115200
                sm.open(port, baud)
            elif cmd[0] == "close":
                sm.close()
            elif cmd[0] == "exit":
                sm.close(); plt.close('all'); break
            else:
                print("Unknown command")

    threading.Thread(target=console_loop, daemon=True).start()
    plt.show()
