"""
Should contain stub classes that are useful for testing.
"""


class StubThreadsafeSerialWriter:
    """
    Stub class for simulating a ThreadsafeSerialWriter. When a message is
    written, it doesn't actually write to the serial port. Instead, it
    stores the messages for testing purposes. Since python doesn't support
    interfaces, this is the best we can do...

    Attributes:
        subject_written (bytes): The last subject written.
        message_written (bytes): The last message written.
    """
    def __init__(self):
        self.subject_written = bytes()
        self.message_written = bytes()

    def write(self, subject, message):
        self.subject_written = subject
        self.message_written = message
