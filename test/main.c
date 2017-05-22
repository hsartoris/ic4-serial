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
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "isense.h"

int main() {
  int fd = posix_openpt(O_RDWR | O_NOCTTY);
  printf("%s\n", ptsname(fd));
  
  int serial = open("/dev/ttys004", O_RDWR| O_NOCTTY | O_NDELAY /*| O_SYNC */);
  struct termios tty;
  struct termios tty_old;
  memset (&tty, 0, sizeof(tty));
  if (tcgetattr(serial, &tty) != 0) {
    //printf("Error" + errno + " from tcgetattr: " + strerror(errorno) + endl);
    printf("you may need to elevate privileges\n");
  }
  tty_old = tty;
  cfsetospeed(&tty, (speed_t)B38400);
  cfsetispeed(&tty, (speed_t)B38400);
  tty.c_cflag     &=  ~PARENB;            // Make 8n1
  tty.c_cflag     &=  ~CSTOPB;
  tty.c_cflag     &=  ~CSIZE;
  tty.c_cflag     |=  CS8;

  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cc[VMIN]   =  1;                  // read doesn't block
  tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

  cfmakeraw(&tty);

  tcflush(serial, TCIFLUSH);
  if (tcsetattr(serial, TCSANOW, &tty) != 0) {
    printf("you definitely need to elevate privileges\n" );
    //std::cout << "Error " << " from tcsetattr" << setd::endl;
  }

  Bool loop = FALSE;
  int i;
  ISD_TRACKING_DATA_TYPE data;
  ISD_TRACKER_HANDLE handle = 0;
  ISD_TRACKER_INFO_TYPE tracker;
  int attributes = 3; // change based on what exactly is being sent

  handle = ISD_OpenTracker((Hwnd)NULL, 0, FALSE, FALSE );
  if ( handle > 0 ) {
    printf( "\n Az El Rl X Y Z \n" );
    loop = TRUE;
  }
  else {
    printf( "Tracker not found" );
    return -1;
  }
  unsigned char out[50];
  int precision=10;
  char out2[precision];
  char* comma = ',';
  char* endl = '\r\n';
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

      sprintf(out, "%f", data.Station[0].Euler[0]);
      strncpy(out2, out, precision-1);
      out2[precision-2] = '\n';
      out2[precision-1] = '\0';
      write(serial, &out2, precision);
      tcdrain(serial);
      //write(serial, data.Station[0].Euler[0], sizeof(data.Station[0].Euler[0])+1);
      //write(serial, &endl, 2);
      //tcdrain(serial);
      /*
	for (int i = 0; i < sizeof(data.Station[0].Euler)/sizeof(float); i++) {
	sprintf(out, "%f", data.Station[0].Euler[i]);
	strncpy(out2, out, precision-1);
	out2[precision-1] = '\0';
	write(serial, &out2, precision);
	tcdrain(serial);
	write(serial, &comma, 1);
	}
	write(serial, &endl, 2);
      */

      ISD_GetCommInfo( handle, &tracker );
      printf( "%5.2f Kb/s %d Rec/s \r", tracker.KBitsPerSec, tracker.RecordsPerSec );
      fflush(0);
    }
    //usleep(1/baudrate);
  }

  ISD_CloseTracker(handle);
  return 0;
}
