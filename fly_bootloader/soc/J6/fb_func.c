#include "../soc.h"

void *fb_base_get()
{
    return gd->fb_base;
}
