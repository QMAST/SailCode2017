# Raspberry Pi Software

This file should contain documentation for the Raspberry Pi code.

## How to setup dev environment
_Add virtualenv instructions here_

## Class overview
![class-diagram](./img/class_diagram_v1.png)

#### microcontrollerIO.py - ThreadsafeSerialWriter
Writes to a serial port in a thread safe manner. If two threads try to write to the same port, then the messages will be send properly without getting jumbled together. There should ever only be one instance of this class per serial port. All objects that write to the same port should share the ThreadSafeSerialWriter instance. 

The `write` method takes a subject and message, and sends it to a serial port

#### microcontrollerIO.py - SerialReader
Reads a data of the form `XXXMESSAGE;` from the serial port. The data will be split into the subject and message (not including the semicolon) and will be sent to the `State`.

#### state.py - State
Contains the state of the all the sensors. There should only ever be one instance of this.
        
A state has handlers,which should be thought of as `reacting` to messages from the serial port.

For instance, the `_handle_compass` method is called every time a message with subject CP is recieved, and it reacts by updating the `compass_angle` variable. Handlers should not be hugely complex, and should never need to be explicitly called. More complicated logic (autopilot, manual control) should be handled by the Controller, which can read from the State.


#### automatic\_control.py - Controller
This class has access to the ThreadSafeSerialWriter. It should provide abstracted methods that allow control of the boat (e.g. actuate winch)

## Bytes in Python
Unlike languages like C or C++, a Python `str` is not treated as an array of 8-bit bytes. Python has two built-in types for handling bytes - `bytes` and `bytearray`. This means that if someone wants to send a string to any serial i/o, one must convert into `bytes`. There are multiple ways to do this but for consistency, one should use the `encode()` method.

#### Converting string to bytes
```
my_string = "hello"
my_string_as_bytes = my_string.encode('utf-8')  # utf-8 is a standard way of encoding text into bytes.
```

#### Converting numbers to bytes
```
my_num = 123

# Convert my num to a string, then convert it to a byte
my_num_as_bytes = str(my_num).encode('utf-8')
```

If you ever have a string literal, you can use the `b` prefix, which will encode to ascii by default (which utf-8 is backwards compatible with.)

```
my_byte_message_1 = b"hello"  # Preferred
my_byte_message_2 = "hello".encode('utf-8')  # Also valid, but more tedious to write.
```
