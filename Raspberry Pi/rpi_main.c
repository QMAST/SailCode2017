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

        printf("Bytes read: %i, buffer contains:\n", n);
        for (int i = 0; i < n; i++)
        {
            printf("%i\n", buf[i]);
        }
        usleep(500000);
    }
    close(fd);
    return 0;
}