import serial
import logging


class Reader:
    def __init__(self):
        pass

        # Dictionary that will return a callback
        # that will handle a the given message. Note,
        # the message will be a bytearray and will not
        # contain the semicolon
        self.callbacks = {
                "GP": self.handle_GPS,
                "CP": self.handle_compass,
                "TM": self.handle_temperature,
                "WV": self.handle_windvane,
                "PX": self.handle_pixy,
                "LD": self.handle_lidar
            }

    def handle_GPS(self, message):
        pass

    def handle_compass(self, message):
        pass

    def handle_temperature(self, message):
        pass

    def handle_windvane(self, message):
        pass

    def handle_pixy(self, message):
        pass

    def handle_lidar(self, message):
        pass


def read_until_semicolon(io, buffer, buffer_size):
    """
    Reads data from serial into buffer. Returns the number
    of characters written into the buffer
    """
    i = 0
    while i < buffer_size:
        buffer[i] = serial.read(1)
        if buffer[i] == ord(';'):
            return i
        i += 1

    return -1


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
    reader = Reader()
    buffer_size = 1024
    buffer = bytearray(buffer_size)

    while True:
        subject = io.read(2).decode()
        if subject not in reader.callbacks:
            # The subject is malformed
            logging.log("Malformed subject:'{}'".format(subject))
            continue

        # Get the callback that will read the message
        callback = reader[subject]

        # Read the message into the buffer
        bytes_read = read_until_semicolon(io, buffer, buffer_size)
        if bytes_read == -1:
            logging.log(
                "Recieved message does not terminate: '{}'".format(buffer))
            continue

        callback(buffer[:bytes_read])

    io.close()


if __name__ == "__main__":
    main()
