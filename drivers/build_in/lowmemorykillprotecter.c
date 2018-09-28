// #include <../drivers/flyaudio/lowmemorykillprotecter.c>
//char **lmk_white_list = NULL;
//EXPORT_SYMBOL(lmk_white_list);


{
    static int j, skip;
    skip = 0;
    if(lmk_white_list != NULL)
	    for(j = 0; lmk_white_list[j] != NULL; j++)
	    {

		if (strncmp(p->comm, lmk_white_list[j], strlen(lmk_white_list[j]) - 1) == 0)
		{
		    pr_debug("lmkp:protect %s\n", lmk_white_list[j]);
		    skip = 1;
		    break;
		}

		if (strstr(p->comm, "fly"))
		{
		    pr_debug("lmkp:protect %s\n", p->comm);
		    skip = 1;
		    break;
		}

		if (strstr(p->comm, "qualcomm"))
		{
		    pr_debug("lmkp:protect %s\n", p->comm);
		    skip = 1;
		    break;
		}

		if (strstr(p->comm, "lidbg"))
		{
		    pr_debug("lmkp:protect %s\n", p->comm);
		    skip = 1;
		    break;
		}


	    }

    if(skip == 1)
    {
        task_unlock(p);
        continue;
    }
}
