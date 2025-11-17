import time
import struct
import numpy as np
from i2c_interface import I2CInterface

class TelemetryController:
    def __init__(self, i2c_interface: I2CInterface, addr=0x10):
        """
        Initialize telemetry controller with an I2C interface.
        
        Args:
            i2c_interface: An instance of I2CInterface (e.g., MockI2C or MCP2221Driver)
            addr: I2C address of the flight controller (default: 0x10)
        """
        self.i2c = i2c_interface
        self.addr = addr

    def request_telemetry(self):
        """
        Request telemetry data from the flight controller.
        
        Protocol:
        1. Write command 0x01 to request telemetry
        2. Read chunks until all data is received
        
        Returns:
            bytes: Complete telemetry data
        """
        CMD_GET_TELEMETRY = 0x01
        
        # Send command to request telemetry
        self.i2c.write(self.addr, bytes([CMD_GET_TELEMETRY]))
        time.sleep(0.01)  # Small delay for slave to process command

        complete_data = bytearray()
        chunk_idx = 0
        total_chunks = None

        while True:
            # Read one 32-byte packet
            chunk = self.i2c.read(self.addr, 32)

            seq, total, length = chunk[0], chunk[1], chunk[2]
            
            # First chunk tells us total number of chunks
            if chunk_idx == 0:
                total_chunks = total
                if total_chunks == 0:
                    # no data available
                    break
            
            # Extract data portion (skip header: seq, total, length)
            if len(chunk) >= 3 + length:
                data = chunk[3:3 + length]
                complete_data.extend(data)
            else:
                # chunk data incomplete
                break

            chunk_idx += 1
            if chunk_idx >= total_chunks:
                # all chunks received
                break
            time.sleep(0.01)  # Small delay between chunk reads

        return bytes(complete_data)
    
    def request_sensor_data(self):
        return self.request_telemetry()
    
    def parse_telemetry_packet(self, data: bytes):
        """
        Parse the telemetry packet from bytes.
        TelemetryPacket structure (44 bytes):
        - uint32_t timestamp (4 bytes)
        - float latitude (4 bytes)
        - float longitude (4 bytes)
        - float altitude (4 bytes)
        - float verticalVelocity (4 bytes)
        - float heading (4 bytes)
        - float angularVelX (4 bytes)
        - float angularVelY (4 bytes)
        - float angularVelZ (4 bytes)
        - float temperature (4 bytes)
        - uint8_t stage (1 byte)
        - uint8_t gpsFixQuality (1 byte)
        - uint8_t reserved[2] (2 bytes)
        
        Args:
            data: Raw telemetry bytes from I2C
            
        Returns:
            dict: Parsed telemetry data or None if invalid
        """
        if len(data) < 44:
            return None
        
        # Unpack the telemetry packet
        unpacked = struct.unpack('<IfffffffffBB2x', data[:44])
        
        return {
            'timestamp': unpacked[0],
            'latitude': unpacked[1],
            'longitude': unpacked[2],
            'altitude': unpacked[3],
            'verticalVelocity': unpacked[4],
            'heading': unpacked[5],
            'angularVelX': unpacked[6],
            'angularVelY': unpacked[7],
            'angularVelZ': unpacked[8],
            'temperature': unpacked[9],
            'stage': unpacked[10],
            'gpsFixQuality': unpacked[11]
        }
    
    def get_parsed_telemetry(self):
        """
        Request and parse telemetry data from the flight computer.
        
        Returns:
            dict: Parsed telemetry data or None if request/parse failed
        """
        data = self.request_telemetry()
        if data and len(data) > 0:
            return self.parse_telemetry_packet(data)
        return None
    
    def format_telemetry_string(self, telem: dict) -> str:
        """
        Format telemetry data into a string similar to the flight computer's format.
        Format: TELEM2/<time>,<altitude>,<velocity>,<acceleration>,<lat>,<lon>
        
        Args:
            telem: Parsed telemetry dictionary
            
        Returns:
            str: Formatted telemetry string
        """
        # Calculate acceleration magnitude from angular velocity (approximation)
        acc_magnitude = np.sqrt(telem['angularVelX']**2 + 
                               telem['angularVelY']**2 + 
                               telem['angularVelZ']**2)
        
        # Convert timestamp from ms to seconds
        time_sec = telem['timestamp'] / 1000.0
        
        return f"TELEM2/{time_sec:.3f},{telem['altitude']:.2f},{telem['verticalVelocity']:.2f}," \
               f"{acc_magnitude:.2f},{telem['latitude']:.7f},{telem['longitude']:.7f}\n"
    
    def get_formatted_telemetry(self) -> str:
        """
        Request, parse, and format telemetry data from the flight computer.
        
        Returns:
            str: Formatted telemetry string or None if request/parse failed
        """
        telem = self.get_parsed_telemetry()
        if telem:
            return self.format_telemetry_string(telem)
        return None
