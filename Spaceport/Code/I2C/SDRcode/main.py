from telemetry_controller import TelemetryController
from mock_i2c import MockI2C
import time

def main():
    i2c = MockI2C()  # replace with MCP2221Driver() later
    telemetry = TelemetryController(i2c)

    while True:
        data = telemetry.request_sensor_data()
        # what needs to happen with the telemetry data here?
        time.sleep(0.5)

if __name__ == "__main__":
    main()