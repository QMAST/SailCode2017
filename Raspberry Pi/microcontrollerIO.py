import threading
import logging


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
        self.buffer_size = 128
        self.state = state

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


def startSerialReader(port, state):
    """
    Code for running a serial reader that will update the state. This
    method will be run on its own thread thread.
    """

    logging.info("Starting serial reader thread.")
    reader = SerialReader(port, state)
    while True:
        reader.listen()
