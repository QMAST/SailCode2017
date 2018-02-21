# Cross Device Communication
#### for the 2017-2018 Boat System
## Introduction
The 2017-2018 boat introduces the use of a Raspberry Pi 3 B ("RPi") for increased processing capability for more complex navigation control. However, the onboard Arduino Mega ("Mega") board has been retained for its many IO pins and flexibility.
In the current setup, the Mega controls access to all the sensors, servos, the RC and the XBee module. The RPi taps into the Mega's sensors to perform calculations required for autonomy and provides instruction back to the Mega. The XBee module behaves as a wireless serial line, allowing land-based monitoring and control of the system. The Mega is thus required to choreograph the transmission of messages between the RPi and XBee in addition to executing and responding to requests by both parties.
This document will describe the physical wiring configuration of the three systems (Mega, RPi and XBee) and the common communication format for sending and requesting data and status and controlling devices. 
## Wiring Configuration
### Mega to RPi
The RPi GPIO 14 is connected to the Mega RX2. RPi GPIO 15 is connected to Mega TX2. The connection uses 9600 baud, 8 data bits, no parity, and 1 stop bit. Because the RPi GPIO pins operate at 3.3V (not 5V tolerant) and the Mega pins at 5.0V, a voltage divider is placed between GPIO 15 and TX2. The Mega registers 3.3V as HIGH so no level conversion is needed.
### Mega to XBee
The Mega TX1/RX1 are connected to the XBee over serial TTL at 115200 baud.
## Transmission Format
Example: `XXMESSAGE;`
The devices share a common communication format for simplicity. Each message consists of a string terminated by a **semicolon**. Do not use line break characters or message contents may be invalidated. 
The first two characters of the string represent the subject of the message. A full list of subjects and their corresponding meanings/sensor can be found below, along with the expected response. The remainder of the string up to the semicolon is the message block. 
The message portion cannot start with `?` unless the transmission is changing sensor update frequency
Any transmissions that are corrupted or malformed are ignored. Currently, there is no way to confirm receipt and proper execution of a command (outside of querying), which may be a problem with remote, XBee based transmissions. Error correction and reporting may be added in the future. 
## Subjects and Messages
The following section describes the meaning of different message subjects and how the device should respond if it receives that kind of string. Where XBee is indicated, it is assumed that a land-based control device will receive and respond as expected.
### Sensors
By default, the Mega will transmit the message `00X;` every 3 seconds to the RPi and XBee (described below), along with all sensor data. To receive data faster or slower than this from a specific sensor, the RPi or XBee can send the message `XX?x;`, where `XX` is the subject code for the sensor and `x` is the time between each transmission in milliseconds. If `x` is 0, data from that sensor will not be transmitted to the requesting device.
#### GP - GPS (Adafruit Ultimate GPS Breakout)
The Mega will send `GP1;` if the module is online and searching for a GPS fix and `GP0;` if the module is unresponsive. The Mega will send coordinates in decimal format. For example, `GP44.225279,-76.497329;` corresponding to 44°13'31.0"N 76°29'50.4"W. Six decimal points will be reported (%.6f).
#### CP - Compass (CMPS11)
The Mega will send `CPX;` with X being a degree (0-359) representing the boat's bow relative to magnetic north.
#### TM - Temperature (CMPS11)
The Mega will send `TMX;` with X the box temperature in degrees Celsius.
#### WV - Wind Vane (Model Unknown)
The Mega will send `WVX;` with X being a degree (0-359) representing the current wind direction relative to the boat's bow.
#### PX - Pixy Camera (CMUcam5)
The Mega will send PX0 if no object is detected. The Mega will send `PXX;` with X being a section (1-5) representing the objects location in the Pixy's view with 1 being the far left section and 5 being the far right section. **Not yet implemented.**
#### LD - LIDAR (Model Unknown)
**Not yet implemented, specification unknown.**
### 0x Series – Diagnostics and Basic Features
#### 00 - Device Online/Mode
The Mega sends `00X;` with X indicating the device state (0 = Error/Disabled, 1 = Remote Control (Default), 2 = Autopiloted by RPi) to the RPi and XBee every 3 seconds while on. 

`00?;` is used to check if the RPi/Xbee is online. Once received, the device should respond with `001;`. The Mega will send `00?;` to check RPi/Xbee response every 5 seconds initially and if the device misses a response. Once the connection is initially established, the Mega will send `00?;` every 10 seconds to verify that the device is still alive.

Received by | Action/Response 
---|---
Mega | Saves the last response time of the RPi/XBee. The Mega ignores `00?;` messages.
RPi | Responds to the Mega's `00?;` with `001;`
XBee | Responds to the Mega's `00?;` with `001;`
#### 01 - Device Powered On
`011;` is sent by the Mega to the RPi/XBee as it is powering on. 

Received by | Action/Response 
---|---
Mega | No action/response (May change in the future)
RPi | Pauses, resubscribes to data and resumes navigation if Mega lost power while in autopilot (brownout).
XBee | Updates GUI
#### 02 - Blink LED Strip
When `02X;` is received by the Mega it will blink the LED strip in a pattern corresponding to X. Currently only `021;` is implemented.

Received by | Action/Response 
---|---
Mega | Blink LEDs
#### 03 - Enable/Disable Remote Control
`031;` to enable and `030;` to disable RC control of rudders and sails.

Received by | Action/Response 
---|---
Mega | Enable/disable remote control
#### 05 - General Log Message
Sent by the RPi to the Mega and relayed or by the Mega directly to the XBee to be displayed in the console and saved in the log.

Received by | Action/Response 
---|---
Mega | Relay message from the RPi to the XBee.
XBee | Update GUI, show error notification
#### 07 - General Error Message
Sent by the RPi to the Mega and relayed or by the Mega directly to the XBee to display a error message to the land-based operator.

Received by | Action/Response 
---|---
Mega | Relay message from the RPi to the XBee.
XBee | Update GUI, show error notification
#### 08 - Mega Error
`081;` is sent by the Mega to the RPi/XBee if there is a critical failure that does not allow it to continue. Not currently implemented.

Received by | Action/Response 
---|---
RPi | Stops autopilot
XBee | Updates GUI, show error notification
#### 09 - RPi Status
The Mega sends `09X;` with X indicating the RPi device state (0 = Offline, 1 = Online, 2 = Error) to the XBee every 3 seconds. RPi status is determined through `00?;` (device online) pings.

Received by | Action/Response 
---|---
XBee | Updates GUI, show error notification
### Ax Series – Autopilot Features
Many of these messages are sent via the XBee to the Mega and then relayed to the RPi. **For all of the following subjects, the XBee can query the current state/stored variable for a subject by sending `AX?;` or something similar, as described below.** It is therefore important that the RPi also responds to these requests by responding appropriately with an answer or 0 if empty.
#### A0 - Enable/Disable Autopilot
`A01;` to enable and `A00;` to disable autopilot.
> **Warning**: Enabling autopilot by sending `A01;` to the Mega via XBee does not guarantee the system will switch into autopilot. If it does, however, RC control will be disabled.

If the XBee sends `A01;` to enter autopilot, the Mega will send `A01;` to the RPi. If the RPi has all the information needed to enter autopilot, it send `A01;` back and take over control. However, if the RPi is missing information (waypoints, GPS, compass data), no actions will occur.

Received by | Action/Response 
---|---
Mega | If sent by the RPi, the Mega will change it's internal variables relating to it's current mode, enable/disable remote control and relay `A0X;` back to the XBee. If sent by the XBee, it will relay the request to the RPi.
RPi | Assess readiness for autopilot and responds with `A01;` if ready, then takes over control.
XBee | Update GUI.
#### A1 - Autopilot Mode
The XBee will send a message in the format `A1X;` with X corresponding to the autopilot mode.

Mode (X) | Description 
---|---
0 | Basic navigation. Navigate to each waypoint and disable autopilot when the last waypoint is reached.
1 | Looping navigation. Navigate to each waypoint, going to the first when the last waypoint is reached.
2 | Stationkeeping. Follow the challenge description for the Sailbot Competition.
2 | Search. Follow the challenge description for the Sailbot Competition.

Received by | Action/Response 
---|---
Mega | Relay the request to the RPi if received from XBee. Relay the mode to the XBee if received from the RPi.
RPi | Appropriately adjust autopilot mode. Respond with current mode in format `A1X;` if `A1?;` is received.
XBee | Update GUI.
#### A2 - Waypoint
The XBee will send a message in the format `A2XX,LAT,LONG;` with X corresponding to the waypoint to overwrite (01-10) or `A2?XX;` with X corresponding to the waypoint to read (01-10).

Received by | Action/Response 
---|---
Mega | Relay the request to the RPi if received from XBee. Relay fulfilled read requests from the RPi to the XBee.
RPi | Read/write waypoint as appropriate. If it is a read request, respond with waypoint in format `A2XX,LAT,LONG;`.
XBee | Update GUI.
#### A3 - Active Waypoint Range
In certain navigation challenges, not all the defined waypoints are to be navigated to. For example, maybe navigation should stop after navigating through waypoints 01-03.
The XBee will send a message in the format `A3SSFF;` with SS corresponding to the waypoint to start on and FF to the waypoint to end on (01-10). The XBee can also send or `A3?;` to request the active waypoint range.

Received by | Action/Response 
---|---
Mega | Relay the request to the RPi if received from XBee. Relay fulfilled read requests from the RPi to the XBee.
RPi | Read/write waypoint range as appropriate. If it is a read request, respond with waypoint range in format `A3SSFF;`.
XBee | Update GUI.
#### A8 - Current Waypoint
Sent by the RPi to the Mega to the XBee in the format `A8XX;` where XX is the index of the current waypoint being navigated to.

Received by | Action/Response 
---|---
Mega | Relay message from the RPi to the XBee.
XBee | Update GUI.
#### A9 - General Autopilot Status
Sent by the RPi to the Mega to the XBee to display as a status message.

Received by | Action/Response 
---|---
Mega | Relay message from the RPi to the XBee.
XBee | Update GUI.
### Sx Series – Servo
Can be sent by the RPi or XBee to the Mega to actuate the servos.
#### SW - Winch
`00XXX;` is used to move the winch where 0 is fully sheeted in and 100 is fully sheeted out. Do not pad number with zeros.
#### SW - Rudder
`00XXX;` is used to move the rudder between 0 to 180 degrees where 0 corresponds with fully turned clockwise, 180 fully turned counterclockwise and 90 being the "zero" rudder position. Do not pad number with zeros.
