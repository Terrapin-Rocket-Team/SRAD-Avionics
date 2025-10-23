import os, io, time
import numpy as np
from gnuradio import gr

try:
    import serial
except Exception:
    serial = None

class blk(gr.sync_block):
    """
    Byte stream source for GNU Radio (file or serial/UART)

    Parameters:
      path        : "/dev/ttyACM0" (serial) or "/path/to/file.txt"
      mode        : "serial" or "file"
      baud        : serial baud (ignored for file)
      repeat      : loop file on EOF (files only)
      chunk       : max bytes per work() attempt
      timeout_ms  : serial read timeout (0 = non-blocking)
      text        : if True and mode="file", open in text mode and encode to bytes
      encoding    : text encoding (e.g., "utf-8", "latin-1"); only used when text=True
      normalize_nl: if True (text mode), convert all newlines to '\n'
    """
    def __init__(self, path="/dev/ttyACM0", mode="serial", baud=115200,
                 repeat=False, chunk=4096, timeout_ms=0,
                 text=False, encoding="utf-8", normalize_nl=True):
        gr.sync_block.__init__(
            self, name="byte_stream_source", in_sig=None, out_sig=[np.uint8]
        )
        self.path = str(path)
        self.mode = str(mode).lower()
        self.baud = int(baud)
        self.repeat = bool(repeat)
        self.timeout = max(0, int(timeout_ms))

        self.text = bool(text)
        self.encoding = str(encoding)
        self.normalize_nl = bool(normalize_nl)

        self._fh = None
        self._is_serial = (self.mode == "serial")
        self._eof = False

    def start(self):
        if self._is_serial:
            if serial is None:
                raise RuntimeError("pyserial not available; install it or use mode='file'")
            self._fh = serial.Serial(
                self.path, self.baud,
                timeout=self.timeout / 1000.0
            )
            time.sleep(0.500)
            self._fh.write("ping\n".encode())
            time.sleep(0.5)
            if(self._fh.in_waiting > 0):
                a = self._fh.readline().decode().strip()
                print(a)
                if(a == 'pong'):
                    print('Recieved Handshake!')
                else:
                    print('No Pong')
            else:
                print('No Response')    
            self._fh.reset_input_buffer()
        else:
            if self.text:
                # newline=None enables universal newlines; we'll optionally normalize
                self._fh = open(self.path, "r", encoding=self.encoding, newline=None)
            else:
                self._fh = open(self.path, "rb", buffering=0)
            self._eof = False
        return super().start()

    def stop(self):
        try:
            if self._fh is not None:
                self._fh.close()
        finally:
            self._fh = None
        return super().stop()

    def _read_line(self):
        if self._is_serial:
            return self._fh.readline()
        else:
            if self._eof and not self.repeat:
                return b""
            if self.text:
                # Read chars, encode to bytes
                s = self._fh.readline()  # chars, not bytes
                if not s:
                    if self.repeat:
                        try:
                            self._fh.seek(0)
                            s = self._fh.readline()
                            self._eof = False
                        except Exception:
                            self._eof = True
                    else:
                        self._eof = True
                        return b""
                if s and self.normalize_nl:
                    s = s.replace("\r\n", "\n").replace("\r", "\n")
                return s.encode(self.encoding, errors="replace")
            else:
                buf = self._fh.readline()
                if not buf:
                    if self.repeat:
                        try:
                            self._fh.seek(0)
                            buf = self._fh.readline()
                            self._eof = False
                        except Exception:
                            self._eof = True
                    else:
                        self._eof = True
                return buf

    def work(self, input_items, output_items):
        out = output_items[0]
        if self._fh is None:
            return 0

        attempts = 0
        while attempts < 4:
            chunk = self._read_line()
            if chunk:
                n = len(chunk)
                out[0:n] = np.frombuffer(chunk, dtype=np.uint8, count=n)
                break
            else:
                attempts += 1
                time.sleep(0.001)
        return n
