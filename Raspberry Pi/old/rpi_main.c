#include <stdlib.h>     
#include <stdio.h>      // Standard input/output definitions
#include <stdint.h>     // Standard types
#include <string.h>     // String function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <sys/ioctl.h>

#define SERIAL_PORT "/dev/ttyACM0"  // TODO: Pass in as command line option?
#define BAUD_RATE B57600
// TODO: Add global variables to be used in the sail logic

int set_interface_attributes(int fd)
{
    struct termios tty;    

    if (tcgetattr(fd, &tty) < 0)
    {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    tty.c_cflag |= (CLOCAL | CREAD);  ignore modem controls 
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8-bit characters
    tty.c_cflag &= ~PARENB;  // no parity bit
    tty.c_cflag &= ~CSTOPB;  // only need 1 stop bit
    tty.c_cflag &= ~CRTSCTS; // no hardware flowcontrol

    // setup for non-canonical mode
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    // fetch bytes as they become available
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
}

int main(int argc, char *argv[])
{
    int fd;
    
    // Open serial port
    fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    printf("QMAST Control\n");
    printf("fd opened as %i\n", fd);
    
    usleep(100000);                 // Wait for the Arduino to reboot
    set_interface_attributes(fd);   // Setup serial interface attributes

    // Read from device
    tcflush(fd, TCIFLUSH);
    while (1)
    {
        char buf[1] = {};
        int n = read(fd, buf, 1);
		char* subject;
		char* message;
		int j=0, i=0;
		//2 while loops used to ensure that string being read is between 2 ;'s
		//May need to be updated in testing
		while(buf[j]!=';')
		{
			j++;
		}
        printf("Bytes read: %i, buffer contains:\n", n);
		char subject[0]= buf[j+1];
		char subject[1]= buf[j+2];
		while(buf[j]!=';')
		{
			message[i]=buf[j];
			j++;
			i++;
		}
		//TODO: Fill out else ifs to assign sensor values, perform diagnostic features and to configure autopilot. Some need to call tranmsit function to send
		//infor back to the mega
		//Basic Sensors
		//GPS
		if(strcmp(subject, "GP"))
		{
			
		}
		//Compas
		if(strcmp(subject, "CP"))
		{
			
		}
		//Temperature
		else if(strcmp(subject, "TM"))
		{
			
		}
		//Wind Vane
		else if(strcmp(subject, "WV"))
		{
			
		}
		//Pixy
		else if(strcmp(subject, "PX"))
		{
			
		}
		//Lidar
		else if(strcmp(subject, "LD"))
		{
			
		}
		//Diagnostic and basic features
		//Device Online/Mode
		else if(strcmp(subject, "00"))
		{
			
		}
		//Device Powered on
		else if(strcmp(subject, "01"))
		{
			
		}
		//Mega Error
		else if(strcmp(subject, "08"))
		{
			
		}
		//Autopilot Features
		//Enable/Disable Autopilot
		else if(strcmp(subject, "A0"))
		{
			
		}
		//Autopilot mode
		else if(strcmp(subject, "A1"))
		{
			
		}
		//Waypoint
		else if(strcmp(subject, "A2"))
		{
			
		}
		//Active waypoint range
		else if(strcmp(subject, "A3"))
		{
			
		}
		
		
        usleep(500000);
    }
    close(fd);
    return 0;
//TODO: Complete function to transmit back to the mega given the inputs
void transmit(char* subject, char* message)
{
	
}
//TODO: Copy over and update Navscore from old sailcode arduino code
void sailLogic()
{
	
}