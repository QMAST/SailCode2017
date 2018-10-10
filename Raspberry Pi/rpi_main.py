import serial
import logging


class Reader:
    def __init__(self, io):
        pass

        # Dictionary that will return a callback
        # that will handle a the given message. Note,
        # the message will be a bytearray and will not
        # contain the semicolon
        self.callbacks = {
                "GP": self._handle_GPS,
                "CP": self._handle_compass,
                "TM": self._handle_temperature,
                "WV": self._handle_windvane,
                "PX": self._handle_pixy,
                "LD": self._handle_lidar,
                "00": self._handle_device_mode
            }

        # Serial object that can be used to relay messages
        # back to the MEGA.
        self.io = io
        self.buffer_size = 1024
        self.buffer = bytearray(self.buffer_size)

    def _handle_GPS(self, message):
        pass

    def _handle_compass(self, message):
        pass

    def _handle_temperature(self, message):
        pass

    def _handle_windvane(self, message):
        pass

    def _handle_pixy(self, message):
        pass

    def _handle_lidar(self, message):
        pass

    def _handle_device_mode(self, message):
        pass

    def _read_until_semicolon(self):
        """
        Reads data from serial into buffer. Returns the number
        of characters written into the buffer
        """
        i = 0
        while i < self.buffer_size:
            self.buffer[i] = self.io.read(1)
            if self.buffer[i] == ord(';'):
                return i
            i += 1

        return -1

    def listen(self):
        subject = self.io.read(2).decode()
        if subject not in self.callbacks:
            # The subject is malformed
            logging.log("Malformed subject:'{}'".format(subject))
            return

        # Read the message into the buffer
        bytes_read = self._read_until_semicolon()
        if bytes_read == -1:
            logging.log(
                "Recieved message does not terminate: '{}'"
                .format(self.buffer))

        self.callbacks[subject](self.buffer[:bytes_read])

        return


def main():
    # Define settings for the serial port
    serial_port = "/dev/ttyACM0"
    baudrate = 9600
    bytesize = serial.EIGHTBITS
    parity = serial.PARITY_NONE
    stopbits = serial.STOPBITS_ONE

    # Create the serial object that willl handle io
    io = serial.Serial(serial_port, baudrate, bytesize, parity, stopbits)

    # Create the object that reads serial data
    reader = Reader(io)
    while True:
        reader.listen()

    io.close()


if __name__ == "__main__":
    main()
