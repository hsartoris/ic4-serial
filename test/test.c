#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main() {
  printf("actually fucking started\n");
  int serial = open("/dev/ttys004", O_RDWR | O_NOCTTY | O_NDELAY);
  printf("got this far 1\n");
  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(serial, &tty) != 0) {
    //printf("Error" + errno + " from tcgetattr: " + strerror(errorno) + endl);
    printf("you may need to elevate privileges\n");
  }
  printf("got this far 2\n");
  cfsetospeed(&tty, (speed_t)B9600);
  cfsetispeed(&tty, (speed_t)B9600);
  tty.c_cflag     &=  ~PARENB;            // Make 8n1
  tty.c_cflag     &=  ~CSTOPB;
  tty.c_cflag     &=  ~CSIZE;
  tty.c_cflag     |=  CS8;

  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cc[VMIN]   =  1;                  // read doesn't block
  tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
  printf("got this far 3\n");

  cfmakeraw(&tty);

  tcflush(serial, TCIFLUSH);
  if (tcsetattr(serial, TCSANOW, &tty) != 0) {
    printf("you definitely need to elevate privileges\n" );
    //std::cout << "Error " << " from tcsetattr" << setd::endl;
  }
  printf("got this far\n");

  float f = 12.3;
  unsigned char str[] = "test\n";
  int inc;
  char* str2 = "test2\r\n";
  //while (1) {
  write(serial, str, sizeof(str)-1);
    //}
    return 0;
}
