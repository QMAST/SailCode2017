import unittest
import sys
sys.path.append('..')
from automatic_control import Controller  # noqa: E402
from state import State  # noqa: E402
from stubs import StubThreadsafeSerialWriter  # noqa: E402


class TestController(unittest.TestCase):
    """
    Tests that the controller writes the correct messages.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.writer = StubThreadsafeSerialWriter()
        self.controller = Controller(self.writer)

    def test_actuate_winch(self):
        self.controller.actuate_winch(55)
        self.assertTrue(self.writer.subject_written == b'00')
        self.assertTrue(self.writer.message_written == b'55')

    def test_CP(self):
        self.controller.actuate_rudder(74)
        self.assertTrue(self.writer.subject_written == b'00')
        self.assertTrue(self.writer.message_written == b'74')


if __name__ == "__main__":
    unittest.main()
