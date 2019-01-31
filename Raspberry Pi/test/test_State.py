import unittest
import sys
sys.path.append('..')
from state import State  # noqa: E402
from stubs import StubThreadsafeSerialWriter  # noqa: E402


class TestState(unittest.TestCase):
    """
    Tests that the handler methods update the state object.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.writer = StubThreadsafeSerialWriter()
        self.state = State(self.writer)

    def test_invalid(self):
        self.state.handle_message(b'asdasd', b'asdas')
        self.assertLogs(level='ERROR')

    def test_GP(self):
        self.state.handle_message(b'GP', b'0')
        self.assertFalse(self.state.is_gps_online)

        self.state.handle_message(b'GP', b'1')
        self.assertTrue(self.state.is_gps_online)

        self.state.handle_message(b'GP', b'123.123,3321.233')
        self.assertTrue(self.state.gps_coordinates == (123.123, 3321.233))

        self.state.handle_message(b'GP', b'-123.123,-3321.233')
        self.assertTrue(self.state.gps_coordinates == (-123.123, -3321.233))

    def test_CP(self):
        self.state.handle_message(b'CP', b'43.2')
        self.assertTrue(self.state.compass_angle == 43.2)

    def test_TM(self):
        self.state.handle_message(b'TM', b'23')
        self.assertTrue(self.state.temperature == 23)

    def test_WV(self):
        self.state.handle_message(b'WV', b'21.2')
        self.assertTrue(self.state.windvane_angle == 21.2)

    def test_PX(self):
        self.state.handle_message(b'PX', b'5')
        self.assertTrue(self.state.pixy_location == 5)

    def test_00(self):
        self.state.handle_message(b'00', b'2')
        self.assertTrue(self.state.mega_state == 2)

        self.state.handle_message(b'00', b'?')
        self.assertTrue(self.writer.subject_written == b'00')
        self.assertTrue(self.writer.message_written == b'1')

    def test_01(self):
        self.state.handle_message(b'01', b'0')
        self.assertFalse(self.state.mega_powered_on)
        self.state.handle_message(b'01', b'1')
        self.assertTrue(self.state.mega_powered_on)

    def test_08(self):
        self.state.handle_message(b'08', b'1')
        self.assertTrue(self.state.mega_error)

    def test_A0(self):
        self.state.handle_message(b'A0', b'1')
        self.assertTrue(self.state.rpi_autopilot_enabled)

        self.state.handle_message(b'A0', b'0')
        self.assertFalse(self.state.rpi_autopilot_enabled)

    def test_A1(self):
        self.state.handle_message(b'A1', b'0')
        self.assertTrue(self.state.rpi_autopilot_mode == 0)
        self.state.handle_message(b'A1', b'?')
        self.assertTrue(self.writer.subject_written == b'A1')
        self.assertTrue(self.writer.message_written == b'0')

        self.state.handle_message(b'A1', b'1')
        self.assertTrue(self.state.rpi_autopilot_mode == 1)
        self.state.handle_message(b'A1', b'?')
        self.assertTrue(self.writer.subject_written == b'A1')
        self.assertTrue(self.writer.message_written == b'1')

        self.state.handle_message(b'A1', b'2')
        self.assertTrue(self.state.rpi_autopilot_mode == 2)
        self.state.handle_message(b'A1', b'?')
        self.assertTrue(self.writer.subject_written == b'A1')
        self.assertTrue(self.writer.message_written == b'2')


if __name__ == "__main__":
    unittest.main()
