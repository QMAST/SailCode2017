import logging


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
        try:
            self.callbacks[subject](message)
        except KeyError:
            logging.error(
                "Invalid subject recieved: {}, message: {}".format(
                    subject, message))
        except Exception as e:
            logging.error(e)

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
