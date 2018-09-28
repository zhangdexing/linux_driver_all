#include <stdio.h>
#include <stdlib.h>

static int J6_test_init(void)
{
	printf("J6_test_init\n");

	return 0;
}

static void J6_test_exit(void)
{
	printf("J6_test_exit\n");
}

module_init(J6_test_init);
module_exit(J6_test_exit);

MODULE_AUTHOR("fly, <fly@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");
