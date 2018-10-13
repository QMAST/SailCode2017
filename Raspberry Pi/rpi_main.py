import sys
import serial
import logging
import threading


class State:
    """
    Class that contains and and can update the state of the sailboat. Can be
    shared across classes.

    Args:
        writer (ThreadsafeSerialWriter): The writer to send the messages too

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

        rpi_autopilot_enabled (bool): Whether autopilot is enabled for the rpi.

        rpi_autopilot_mode (int): The current autopilot mode of the RPI.

        callbacks (dict): Dictionary that will return a callback that will
            handle the given message based on the subject. Note, the message
            will be a bytearray and will not contain the semicolon.
    """
    def __init__(self, writer):
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
        self.rpi_autopilot_enabled = False
        self.rpi_autopilot_mode = 0

        # The handling for the A2, A3 commands are not implemented yet
        self.callbacks = {
                b'GP': self._handle_GPS,
                b'CP': self._handle_compass,
                b'TM': self._handle_temperature,
                b'WV': self._handle_windvane,
                b'PX': self._handle_pixy,
                b'LD': self._handle_lidar,
                b'00': self._handle_device_mode,
                b'01': self._handle_mega_powering_on,
                b'08': self._handle_mega_error,
                b'A0': self._handle_rpi_autopilot_enable,
                b'A1': self._handle_rpi_autopilot_mode,
            }

        self.writer = writer

    def handle_message(self, subject, message):
        """
        Handles the incoming message.
        """
        self.callbacks[subject](message)

    def _handle_GPS(self, message):
        if len(message) == 1:
            logging.info("Recieved gps state: {}".format(message))
            self.is_gps_online = int(message) == 1
        else:
            logging.info("Recieved gps coordinates: {}".format(message))
            x, y = message.split(b',')
            self.gps_coordinates = (float(x), float(y))

    def _handle_compass(self, message):
        logging.info("Recieved compass angle: {}".format(message))
        self.compass_angle = float(message)

    def _handle_temperature(self, message):
        logging.info("Recieved temperature: {}".format(message))
        self.temperature = float(message)

    def _handle_windvane(self, message):
        logging.info("Recieved windvane angle: {}".format(message))
        self.windvane_angle = float(message)

    def _handle_pixy(self, message):
        logging.info("Recieved pixy location: {}".format(message))
        self.pixy_location = int(message)

    def _handle_lidar(self, message):
        # Not implemented
        pass

    def _handle_device_mode(self, message):
        logging.info("Recieved device mode from mega: {}".format(message))
        if message == b'?':
            logging.info("Sending alive state to mega")
            self.writer.write(b'00', b'1')
        else:
            self.mega_state = int(message)

    def _handle_mega_powering_on(self, message):
        logging.info("Recieved mega powering on: {}".format(message))
        self.mega_powered_on = int(message) == 1

    def _handle_mega_error(self, message):
        logging.info("Recieved error from mega: {}".format(message))
        self.mega_error = int(message) == 1

    def _handle_rpi_autopilot_enable(self, message):
        logging.info("Recieved autopilot enable command: {}".format(message))
        self.rpi_autopilot_enabled = int(message) == 1

    def _handle_rpi_autopilot_mode(self, message):
        logging.info("Recieved autopilot mode command: {}".format(message))
        if message == b'?':
            autopilot_mode_as_bytes = \
                    str(self.rpi_autopilot_mode).encode('utf-8')
            self.writer.write(b'A1', autopilot_mode_as_bytes)
        else:
            self.rpi_autopilot_mode = int(message)


class ThreadsafeSerialWriter:
    """
    Threadsafe serial writer that can be shared across multiple threads.

    This object can be used in multiple places to write to the same serial
    port in a threadsafe manner.

    Args:
        port (serial.Serial): Serial that this class will listen and write to.
            This class should not read from this port ever.
    """
    def __init__(self, port):
        self.port = port
        self.lock = threading.Lock()

    def write(self, subject, message):
        """
        Writes to the serial port in a locking manner.

        Args:
            header (bytes): The subject for the message.

            message (bytes): The message itself.
        """
        try:
            self.lock.acquire()
            self.port.write(subject + message + b';')
        finally:
            self.lock.release()


class SerialReader:
    """
    Listens and reads from a given serial port.

    Args:
        port (serial.Serial): Serial port that this class will listen and write
            to. Though the serial object can be used to write directly to the
            given port, one should use the writer object to send a message for
            thread safety.

        state (State): The state of the sailboat that the messages will be
            relayed to.

    Attributes:
        buffer (bytearray): Buffer where serial data is stored

    """
    def __init__(self, port, state):
        self.port = port
        self.buffer_size = 1024
        self.buffer = bytearray(self.buffer_size)
        self.state = state

    def _read_until_semicolon(self):
        """
        Reads data from serial into buffer. Returns the number
        of characters written into the buffer
        """
        i = 0
        while i < self.buffer_size:
            self.buffer[i] = self.port.read(1)
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
        subject = self.port.read(2)

        # Read the message into the buffer
        bytes_read = self._read_until_semicolon()
        if bytes_read == -1:
            logging.info(
                "Recieved message does not terminate: '{}'"
                .format(self.buffer))
            return

        self.state.handle_message(subject, self.buffer[:bytes_read])


class Controller:
    """
    Class used for direct control of the sailboat.

    Args:
        writer (ThreadsafeSerialWriter): The writer to send the messages too

        state (State): The state of the sailboat.
    """
    def __init__(self, writer, state):
        self.writer = writer
        self.state = state

    def actuate_winch(self, value):
        assert 0 <= value <= 100
        logging.info("Actuating winch: {}".format(value))
        self.writer.write(b'00', str(value).encode('utf-8'))

    def actuate_rudder(self, value):
        assert 0 <= value <= 180
        logging.info("Actuating rudder: {}".format(value))
        self.writer.write(b'00', str(value).encode('utf-8'))


def main():
    # Just log to stdout for now
    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

    # Define settings for the serial port
    serial_port = "/dev/ttyvd"
    baudrate = 9600
    bytesize = serial.EIGHTBITS
    parity = serial.PARITY_NONE
    stopbits = serial.STOPBITS_ONE

    # Create the serial object that will be read and written to.
    io = serial.Serial(serial_port, baudrate, bytesize, parity, stopbits)

    # Create a threadsafe serial writer. It can be shared across multiple
    # threads if need be.
    writer = ThreadsafeSerialWriter(io)

    # Object for keeping track of the state of the sailboat
    state = State(writer)

    # Uses for directly controlling the MEGA (e.g. Servos).
    controller = Controller(writer, state)

    # Test inputs to actuate the sailboat
    controller.actuate_winch(20)
    controller.actuate_rudder(90)

    # Create the object that reads serial data. This can be implemented in
    # a seperate thread if need be.
    reader = SerialReader(io, state)
    while True:
        reader.listen()

    io.close()


if __name__ == "__main__":
    main()
