#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/stat.h>
#include "debug.h"

int main(void)
{
	FILE *fp;
	char readbuf[80];

	// try to open my fifo file. it may already exist from
	// an earlier aborted execution
	fp = fopen(DEBUG_FIFO, "r");

	// if the fopen failed, the fifo file does not exist
	if (NULL == fp)
	{
		// Create the FIFO file
		umask(0);
		if (mknod(DEBUG_FIFO, S_IFIFO|0666, 0))
		{
			fprintf(stderr, "mknod() failed\n");
			return 1;
		}
		fp = fopen(DEBUG_FIFO, "r"); // now open the fifo file
	}

	// while we can read (up to 9) chars from the fifo...
	// Note: readbuf[] will be nul-terminated and will
	// include any newlines read from the pipe. since
	// we are reading so few bytes at once (9) it will
	// take several iterations of the 'while loop' to read
	// any long lines written to the pipe by clients.
	while (NULL != fgets(readbuf, 10, fp))
	{
		// print the string just read to my stdout
		printf("%s", readbuf);
		fflush(stdout);
	}

	fclose(fp); // close the fifo file
	remove(DEBUG_FIFO); // delete the fifo file

	return(0);
}
