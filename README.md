# Sailcode 2017
#### for the 2017-2018 QMAST Boat System
## Introduction
This is the repository containing code for the 2017-2018 boat system developed by the Queen’s Mostly Autonomous Sailboat Team (QMAST), a student led design team at [Queen's University](https://engineering.queensu.ca) that designs and builds autonomous sailboats. 
Currently the repo contains code from the 2016-2017 boat system ([learn more](http://qmast.ca/wp-content/uploads/2017/07/2016-2017-Season-Summary-2.0.pdf)). This season, we are working to adapt this code to improve functionality and implement new features based on new hardware.

Major changes this season include the inclusion of a Raspberry Pi 3 B and the replacement of the Airmar PB100 in favor of multiple dedicated sensors. Although not software-related, a notable improvement is that all electronic components have been consolidated onto one PCB designed by [Michael Laurenzi](https://ca.linkedin.com/in/michael-laurenzi-433936b0).

The code is divided into 3 main sections: Arduino, Raspberry Pi and an Android app. The Arduino Mega acts as the main messenger for the system, communicating and recording activity between all of the sensors, RC and Raspberry Pi and the Android app (over XBee). The Raspberry Pi 3 B performs more advanced computations, such as those required for autopilot and searching. The Android app provides remote telemetry and control for the team on land. Details on how they are connected and "talk" to one another is described in [this document](communication_standards.md).

## Current Systems Goals
* Test communication between all hardware components.
* Improve autopilot sailing logic
* Complete implementation of collision avoidance systems (LIDAR and Pixy)
* and more!
## Learn More
If you're interested in learning more about the system or the QMAST team, contact us on our [website](http://qmast.ca) or [Facebook](https://www.facebook.com/QMAST/). 

This code is made available under a GNU GPLv3 license.
