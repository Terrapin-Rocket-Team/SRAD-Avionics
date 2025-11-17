from telemetry_controller import TelemetryController
from mock_i2c import MockI2C
import time
import numpy as np

# Pluto SDR imports - uncomment when Pluto SDR is available
# import adi

def setup_pluto_sdr(frequency_hz=433e6, sample_rate=1e6, tx_attenuation=0):
    """
    Initialize and configure Pluto SDR for transmission.
    
    Args:
        frequency_hz: Center frequency in Hz (default 433 MHz)
        sample_rate: Sample rate in Hz (default 1 MHz)
        tx_attenuation: TX attenuation in dB (default 0)
    
    Returns:
        Configured Pluto SDR object or None if not available
    """
    try:
        # Uncomment when Pluto SDR is available
        # sdr = adi.Pluto()
        # sdr.tx_rf_bandwidth = int(sample_rate)
        # sdr.tx_lo = int(frequency_hz)
        # sdr.sample_rate = int(sample_rate)
        # sdr.tx_hardwaregain_chan0 = -tx_attenuation
        # sdr.tx_cyclic_buffer = True
        # return sdr
        return None  # Placeholder - uncomment above when Pluto SDR is connected
    except Exception as e:
        print(f"Pluto SDR initialization failed: {e}")
        return None

def modulate_and_transmit(sdr, data_string: str, modulation='qpsk'):
    """
    Modulate the telemetry string and transmit via Pluto SDR.
    
    Args:
        sdr: Pluto SDR object
        data_string: Telemetry string to transmit
        modulation: Modulation scheme ('qpsk', 'bpsk', 'fsk', etc.)
    """
    if sdr is None:
        print(f"[Would transmit] {data_string.strip()}")
        return
    
    # Convert string to bytes
    data_bytes = data_string.encode('utf-8')
    
    # Simple QPSK modulation example
    # In practice, you'd want proper encoding, framing, error correction, etc.
    if modulation == 'qpsk':
        # Convert bytes to bits
        bits = []
        for byte in data_bytes:
            for i in range(8):
                bits.append((byte >> (7-i)) & 1)
        
        # Pad to even number of bits for QPSK (2 bits per symbol)
        if len(bits) % 2:
            bits.append(0)
        
        # Map bits to QPSK symbols
        symbols = []
        for i in range(0, len(bits), 2):
            # Gray code mapping: 00->1+1j, 01->-1+1j, 10->1-1j, 11->-1-1j
            bit_pair = (bits[i] << 1) | bits[i+1]
            symbol_map = {
                0: 1+1j,   # 00
                1: -1+1j,  # 01
                2: 1-1j,   # 10
                3: -1-1j   # 11
            }
            symbols.append(symbol_map[bit_pair])
        
        # Convert to numpy array and scale
        samples = np.array(symbols, dtype=np.complex64)
        samples = samples * (2**14)  # Scale for Pluto SDR (16-bit)
        
        # Upsample/pulse shape (simple example - add proper pulse shaping in production)
        # For now, just repeat samples for longer transmission
        samples = np.repeat(samples, 10)  # Repeat each symbol 10 times
        
        # Transmit
        # sdr.tx(samples)
        print(f"[Would transmit {len(samples)} samples] {data_string.strip()}")
    else:
        print(f"Modulation '{modulation}' not yet implemented")

def main():
    i2c = MockI2C()  # replace with MCP2221Driver() later
    telemetry = TelemetryController(i2c)
    
    # Initialize Pluto SDR
    sdr = setup_pluto_sdr(frequency_hz=433e6, sample_rate=1e6, tx_attenuation=0)
    if sdr is None:
        print("Pluto SDR not available - running in simulation mode")
    
    print("Starting telemetry receiver and transmitter...")
    
    while True:
        try:
            # Get formatted telemetry string from controller
            telem_string = telemetry.get_formatted_telemetry()
            
            if telem_string:
                # Print for debugging
                print(f"Telemetry: {telem_string.strip()}")
                
                # Modulate and transmit via Pluto SDR
                modulate_and_transmit(sdr, telem_string, modulation='qpsk')
            else:
                print("No telemetry data received")
            
            time.sleep(0.5)  # Request telemetry at ~2 Hz
            
        except KeyboardInterrupt:
            print("\nShutting down...")
            break
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(1)

if __name__ == "__main__":
    main()