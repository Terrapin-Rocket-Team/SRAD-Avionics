# for testing only
from i2c_interface import I2CInterface

class MockI2C(I2CInterface):
    def __init__(self):
        self.last_command = None
        self.chunk_index = 0
        self.total_chunks = 5  # Simulate 5 chunks
        self.mock_data = bytes(range(128))  # 128 bytes of test data

    def write(self, addr, data):
        self.last_command = data[0] if len(data) > 0 else None
        if self.last_command == 0x01:
            # Reset chunk index when command received
            self.chunk_index = 0
        print(f"[MOCK] Write to 0x{addr:02X}: {data.hex() if isinstance(data, bytes) else data}")

    def read(self, addr, num_bytes):
        if self.last_command != 0x01:
            # No command received, return empty
            return bytes([0] * num_bytes)
        
        # Simulate chunked response
        chunk_size = 29  # DATA_PER_CHUNK
        offset = self.chunk_index * chunk_size
        remaining = len(self.mock_data) - offset
        data_len = min(chunk_size, remaining)
        
        # Create packet: [chunk_index, total_chunks, data_length, ...data...]
        packet = bytearray([self.chunk_index, self.total_chunks, data_len])
        packet.extend(self.mock_data[offset:offset + data_len])
        
        # Pad to requested size
        while len(packet) < num_bytes:
            packet.append(0)
        
        # Truncate if too long
        packet = packet[:num_bytes]
        
        self.chunk_index += 1
        if self.chunk_index >= self.total_chunks:
            self.chunk_index = 0
            self.last_command = None  # Reset after all chunks sent
        
        print(f"[MOCK] Read from 0x{addr:02X}: chunk {packet[0]}/{packet[1]}, {len(packet)} bytes")
        return bytes(packet)