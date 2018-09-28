#include <fbcon.h>
#include "../soc.h"

void *fb_base_get()
{
    return config->base;
}
