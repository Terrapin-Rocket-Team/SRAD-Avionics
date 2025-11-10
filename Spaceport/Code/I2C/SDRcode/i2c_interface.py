# Interface allowing different bridge chips to be used for I2C communication
from abc import ABC, abstractmethod

class I2CInterface(ABC):
    @abstractmethod
    def write(self, addr: int, data: bytes) -> None:
        """Write bytes to I2C device."""
        pass

    @abstractmethod
    def read(self, addr: int, num_bytes: int) -> bytes:
        """Read bytes from I2C device."""
        pass