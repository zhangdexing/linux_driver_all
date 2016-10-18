#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/resource.h>
#include "lidbg_servicer.h"

#define DEBUG_PRINT_FLAG 1

#define LOG_BYTES   (512)


struct uartConfig
{
    int fd;
    int ttyReadWrite;
    int baudRate;
    char *portName;

    int nread;	//bytes to read
    int nwrite;	//bytes to write
    int wlen;		//lenght have sent
    int rlen;		//lenght have read
};

static struct uartConfig pUartInfo;

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;

    if ( tcgetattr( fd, &oldtio) != 0)
    {
        lidbg("SetupSerial .\n");
        return -1;
    }

    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch ( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch ( nEvent )
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    lidbg("nSpeed=%d\n",nSpeed);
    switch ( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    case 576000:
        cfsetispeed(&newtio, B576000);
        cfsetospeed(&newtio, B576000);
        break;
    case 921600:
        cfsetispeed(&newtio, B921600);
        cfsetospeed(&newtio, B921600);
        break;
    case 3000000:
        cfsetispeed(&newtio, B3000000);
        cfsetospeed(&newtio, B3000000);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }

    if ( nStop == 1 )
        newtio.c_cflag &= ~CSTOPB;
    else if ( nStop == 2 )

        newtio.c_cflag |= CSTOPB;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        lidbg("com set error\n");
        return -1;
    }

    //lidbg("set done!\n");
    return 0;
}

int open_port(int fd, char *portName)
{
    long vdisable;

    pUartInfo.fd = open( portName, O_RDWR | O_NOCTTY);

    if (-1 == pUartInfo.fd)
    {
        lidbg("Can't Open Serial Port\n");
        return(-1);
    }
    else
    {
        lidbg("open %s .....\n", portName);
    }

    if (fcntl(pUartInfo.fd, F_SETFL, 0) < 0)
        lidbg("fcntl failed!\n");
    else
        lidbg("fcntl=%d\n", fcntl(pUartInfo.fd, F_SETFL, 0));
    if (isatty(STDIN_FILENO) == 0)
        lidbg("standard input is not a terminal device\n");
    else
        lidbg("isatty success!\n");

    return pUartInfo.fd;
}

int main(int argc , char **argv)
{

    static pthread_t readTheadId;
    static pthread_t writeTheadId;
    int ret;
    int i = 0;
    char data[32] = {0};

    if(argc < 4)
    {
        lidbg("example: %s /dev/ttyS0 115200 len data[0] data[1] ....\n", argv[0]);
        return -1;
    }

    pUartInfo.portName = argv[1];
    pUartInfo.baudRate = strtoul(argv[2], 0, 0);
    pUartInfo.wlen = strtoul(argv[3], 0, 0);

    for(i = 0; i < pUartInfo.wlen; i++)
        data[i] = strtoul(argv[4 + i], 0, 0);

    pUartInfo.fd = -1;

    if(pUartInfo.portName == NULL)
    {
        lidbg("ERR:tty dev does not exist.\n");
        return -1;
    }

    //lidbg("%s %s %d\n", argv[0], pUartInfo.portName, pUartInfo.baudRate);

    if ((pUartInfo.fd = open_port(pUartInfo.fd, pUartInfo.portName)) < 0)
    {
        lidbg("open_port error\n");
        return -1;
    }

    if ((i = set_opt(pUartInfo.fd, pUartInfo.baudRate, 8, 'N', 1)) < 0)
    {
        lidbg("set_opt error\n");
        return -1;
    }

    if((pUartInfo.fd) > 0) //Tx
    {
	int len;
        len = write(pUartInfo.fd, data, pUartInfo.wlen);
#if DEBUG_PRINT_FLAG
    lidbg("len=%d\n", pUartInfo.wlen);
    for(i = 0; i < pUartInfo.wlen; i++)
       lidbg("0x%x\n", data[i]);
#endif
    }

    close(pUartInfo.fd);

    return 0;
}



