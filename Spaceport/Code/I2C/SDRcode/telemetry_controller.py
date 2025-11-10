import time
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
