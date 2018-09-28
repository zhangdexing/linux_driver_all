
#include "lidbg_servicer.h"


int main(int argc, char **argv)
{

	int ret;
	char  buff[15];
	int fd;

	fd = open("/dev/flyaudio_carback_transpond", O_RDONLY);
	while(1)
	{
		memset(buff,0,15);
		ret = read( fd, buff, sizeof(buff) );

		printf("read:[%s]\n",  buff);
	}
	return 0;
}

