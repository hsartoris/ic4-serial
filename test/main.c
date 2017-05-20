//==================================================================================================
// @author: Hayden Sartoris
// opens an InterSense device and forwards its data from the input serial port to another one
// kinda silly
// see http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
// for more info on serial port stuff, took everything on forwarding from there
//
// seems that Max supports a maximum baudrate of 38400, keep that in mind
//==================================================================================================

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>


#include "isense.h"
#include <unistd.h>
int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

int main() {
  float baudrate = 9600;
  char* portname = "/dev/cu.Bluetooth-Incoming-Port";
  //char* portname = "/dev/tty.Bluetooth-Incoming-Port";
  Bool loop;
  int i;
  ISD_TRACKING_DATA_TYPE data;
  ISD_TRACKER_HANDLE handle;
  ISD_TRACKER_INFO_TYPE tracker;
  int attributes = 3; // change based on what exactly is being sent

  int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    //error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
    printf("Couldn't open tty to output to. try a different one.\n");
    return -1;
  }
  set_interface_attribs (fd, B9600);
  //set_blocking(fd, 0);
  
  handle = ISD_OpenTracker((Hwnd)NULL, 0, FALSE, FALSE );
  if ( handle > 0 ) {
    printf( "\n Az El Rl X Y Z \n" );
    loop = TRUE;
  }
  else {
    printf( "Tracker not found" );
    return -1;
  }
  
  //for (i=0; i < 20; i++) {
  while (loop) {
    if ( handle > 0 ) {
      ISD_GetTrackingData( handle, &data );
	printf( "%7.2f %7.2f %7.2f %7.3f %7.3f %7.3f ",
	data.Station[0].Euler[0],
        data.Station[0].Euler[1],
        data.Station[0].Euler[2],
        data.Station[0].Position[0],
        data.Station[0].Position[1],
        data.Station[0].Position[2] );
	//write(fd, &data.Station[0].Euler[0], sizeof(data.Station[0].Euler[0]));
	//write(fd, "\n", 1);
	//write(fd, &data.Station[0].Euler[1], sizeof(data.Station[0].Euler[1]));
	//write(fd, &data.Station[0].Euler[2], sizeof(data.Station[0].Euler[2]));
	write(fd, "hello\n", 6);
	tcdrain(fd);

	//fwrite(&data.Station[0].Euler[0], 1, sizeof(data.Station[0].Euler[0]), fd);
	//fwrite(&data.Station[0].Euler[1], 1, sizeof(data.Station[0].Euler[1]), fd);
	//fwrite(&data.Station[0].Euler[2], 1, sizeof(data.Station[0].Euler[2]), fd);
      
      ISD_GetCommInfo( handle, &tracker );
      printf( "%5.2f Kb/s %d Rec/s \r", tracker.KBitsPerSec, tracker.RecordsPerSec );
      fflush(0);
    }
    //usleep(1/baudrate);
  }

  ISD_CloseTracker(handle);
  return 0;
}
