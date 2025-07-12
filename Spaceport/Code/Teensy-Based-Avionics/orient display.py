import serial as pyserial
import serial.tools.list_ports
import threading
import queue
import time
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
        self.baro_queue = queue.Queue()         # queue for raw baro readings
        self.estimate_queue = queue.Queue()     # queue for KF estimates

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
            # Parse baro altitude lines
            elif line.startswith('B,'):
                try:
                    b = float(line[2:])
                    self.baro_queue.put(b)
                except ValueError:
                    pass
            # Parse KF estimate lines
            elif line.startswith('E,'):
                try:
                    e = float(line[2:])
                    self.estimate_queue.put(e)
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

    def get_latest_baro(self):
        b = None
        while not self.baro_queue.empty():
            b = self.baro_queue.get()
        return b

    def get_latest_estimate(self):
        e = None
        while not self.estimate_queue.empty():
            e = self.estimate_queue.get()
        return e

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
    tmp = quat_mult(q, vq)
    res = quat_mult(tmp, quaternion_conjugate(q))
    return res[1:]

# === Main script ===
if __name__ == "__main__":
    sm = SerialManager()
    print("Available ports:", sm.list_ports())
    print("Commands: list, open <port> [baud], close, exit")

    # Buffers for rolling plots
    times, raw_baro, kf_est = [], [], []
    acc_times, acc_vals = [], []
    start_time = None

    fig = plt.figure(figsize=(8,12))
    ax3d = fig.add_subplot(311, projection='3d')
    ax_alt = fig.add_subplot(312)
    ax_acc = fig.add_subplot(313)
    ax_alt.set_ylabel('Altitude [m]')
    ax_alt.set_xlabel('Time [s]')
    ax_acc.set_ylabel('Acc Mag [m/s²]')
    ax_acc.set_xlabel('Time [s]')

    def update(frame):
        global start_time
        t_now = time.time()
        if start_time is None:
            start_time = t_now
        t_rel = t_now - start_time

        # Fetch latest data
        q = sm.get_latest_quaternion()
        a_raw = sm.get_latest_accel()
        b_raw = sm.get_latest_baro()
        e_raw = sm.get_latest_estimate()

        # Orientation plot
        if q is not None:
            ax3d.cla()
            ax3d.set_xlim([-1,1]); ax3d.set_ylim([-1,1]); ax3d.set_zlim([-1,1])
            for vec, col in zip([[1,0,0],[0,1,0],[0,0,1]], ['r','g','b']):
                v = rotate_vector(vec, q)
                ax3d.quiver(0,0,0, *v, color=col, length=1)

        # Altitude plot
        if b_raw is not None:
            times.append(t_rel)
            raw_baro.append(b_raw)
            kf_est.append(e_raw if e_raw is not None else np.nan)
            # Keep last 200
            times[:] = times[-200:]
            raw_baro[:] = raw_baro[-200:]
            kf_est[:] = kf_est[-200:]

            ax_alt.cla()
            ax_alt.plot(times, raw_baro, label='Raw Baro')
            ax_alt.plot(times, kf_est, label='KF Estimate')
            ax_alt.legend(loc='upper left')

        # Acceleration plot
        if a_raw is not None:
            acc_times.append(t_rel)
            acc_vals.append(a_raw[2])
            acc_times[:] = acc_times[-200:]
            acc_vals[:] = acc_vals[-200:]

            ax_acc.cla()
            ax_acc.plot(acc_times, acc_vals, label='|Accel|')
            ax_acc.legend(loc='upper left')

    ani = FuncAnimation(fig, update, interval=50)

    # Simple console thread
    def console_loop():
        while True:
            cmd = input('> ').split()
            if not cmd: continue
            if cmd[0] == 'list': print('Ports:', sm.list_ports())
            elif cmd[0] == 'open' and len(cmd)>=2:
                sm.open(cmd[1], int(cmd[2]) if len(cmd)>=3 else 115200)
            elif cmd[0] == 'close': sm.close()
            elif cmd[0] == 'exit': sm.close(); plt.close('all'); break
            else: print('Unknown cmd')

    threading.Thread(target=console_loop, daemon=True).start()
    plt.tight_layout()
    plt.show()
