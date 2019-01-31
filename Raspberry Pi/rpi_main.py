import sys
import serial
import threading
import logging

from state import State
from microcontrollerIO import startSerialReader, ThreadsafeSerialWriter
from automatic_control import startAutomaticControl

if __name__ == "__main__":
    # Just log to stdout for now
    logging.basicConfig(
            stream=sys.stdout, level=logging.DEBUG)

    # Define settings for the serial port
    serial_port = "/dev/tty.Bluetooth-Incoming-Port"
    baudrate = 9600
    bytesize = serial.EIGHTBITS
    parity = serial.PARITY_NONE
    stopbits = serial.STOPBITS_ONE

    # Create the serial object that will be read and written to.
    io = serial.Serial(serial_port, baudrate, bytesize, parity, stopbits)

    try:
        # Object for keeping track of the state of the sailboat
        state = State(ThreadsafeSerialWriter(io))

        # Spawn a daemon thread for reading from serial port.
        serialReaderThread = \
            threading.Thread(
                name="SerialReader",
                target=startSerialReader,
                args=(io, state))
        serialReaderThread.daemon = True
        serialReaderThread.start()

        # Run automatic control on the main thread.
        startAutomaticControl(state)
    except KeyboardInterrupt:
        logging.info("Recieved keyboard interrupt. Closing.")
    finally:
        io.close()
