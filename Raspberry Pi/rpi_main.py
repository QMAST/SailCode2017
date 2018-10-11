import serial
import logging


class State:
    """
    Class that contains the state of the sailboat.

    Attributes:
        is_gps_online (bool): Whether is the gps is online or not.

        gps_coordinates (bool): The GPS cooridnates of the boat.

        compass (float): A degree (0-359) representing the
            The boat's bow relative to magnetic north.

        temperature (float): The current temperature in celsius.

        windvane_angle (float): A degree (0-359) representing the
            current wind direction relative to the boat's bow.

        pixy_location (int): Value between 1-5 that stores represents
            the objects in the pixy's view.

        mega_state (int): 0 = Error/Disabled, 1 = Remote Control (Default),
            2 = Autopiloted by RPi

        mega_powered_on (bool): True if  the mega is powered on.

        mega_error (bool): True if the mega sends a state of critical failure.
    """
    def __init__(self):
        self.is_gps_online = False
        self.gps_coordinates = (0, 0)
        self.compass_angle = 0
        self.temperature = 0
        self.windvane_angle = 0
        self.pixy_location = 0
        self.mega_state = 0
        self.critical_failure = False
        self.mega_powered_on = False
        self.mega_error = False


class Reader:
    """
    Listens and reads from a given serial port.

    Args:
        io (serial.Serial): Serial that this class will listen and write to.

    Attributes:
        callbacks (dict): Dictionary that will return a callback that will
            handle the given message. Note, the message will be a bytearray
            and will not contain the semicolon

        buffer (bytearray): Buffer where serial data is stored

        state (State): The state of the sailboat.
    """
    def __init__(self, io):
        self.callbacks = {
                "GP": self._handle_GPS,
                "CP": self._handle_compass,
                "TM": self._handle_temperature,
                "WV": self._handle_windvane,
                "PX": self._handle_pixy,
                "LD": self._handle_lidar,
                "00": self._handle_device_mode,
                "01": self._handle_mega_powering_on,
                "08": self._handle_mega_error,
            }

        # Serial object that can be used to relay messages
        # back to the MEGA.
        self.io = io
        self.buffer_size = 1024
        self.buffer = bytearray(self.buffer_size)

        self.state = State()

    def _handle_GPS(self, message):
        if len(message) == 1:
            logging.log("Recieved gps state: {}".format(message))
            self.state.is_gps_online = int(message) == 1
        else:
            logging.log("Recieved gps coordinates: {}".format(message))
            x, y = self.message.split(b',')
            self.state.gps_coordinates = (float(x), float(y))

    def _handle_compass(self, message):
        logging.log("Recieved compass angle: {}".format(message))
        self.state.compass_angle = float(message)

    def _handle_temperature(self, message):
        logging.log("Recieved temperature: {}".format(message))
        self.state.temperature = float(message)

    def _handle_windvane(self, message):
        logging.log("Recieved windvane angle: {}".format(message))
        self.state.windvane_angle = float(message)

    def _handle_pixy(self, message):
        logging.log("Recieved pixy location: {}".format(message))
        self.state.pixy_location = int(message)

    def _handle_lidar(self, message):
        # Not implemented
        pass

    def _handle_device_mode(self, message):
        logging.log("Recieved device mode from mega".format(message))
        if message == b'?':
            logging.log("Sending alive state to mega")
            self.io.write(b"001;")
        else:
            self.state.mega_state = int(message)

    def _handle_mega_powering_on(self, message):
        self.state.mega_powered_on = int(message) == 1

    def _handle_mega_error(self, message):
        self.state.mega_error = int(message) == 1

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
        """
        Listens for a single message in the form XXMESSAGE; from the provided
        serial object. This function should be called in a while loop in order
        to read messages forever.
        """
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

    # Create the object that reads serial data. This can be implemented in a
    # seperate thread if need be.
    reader = Reader(io)
    while True:
        reader.listen()

    io.close()


if __name__ == "__main__":
    main()
