#include "lidbg.h"

static LIST_HEAD(lidbg_list);
static LIST_HEAD(flyaudio_list);
static LIST_HEAD(flyaudio_hal_list);
LIDBG_DEFINE;

struct judgment
{
    char *key;
    int value;
    char *pvalue;
};
struct judgment judgment_list[] =
{
    {"platform_id", -1, NULL},
    {"Factory", -1, NULL},
    {"gboot_mode", -1, NULL},
    {"is_uart_print_enable", -1, NULL},
    {"car_type", -1, NULL},
};
void judgment_list_print(void)
{
    int cunt, size;
    size = ARRAY_SIZE(judgment_list);
    for(cunt = 0; cunt < size; cunt++)
    {
        LIDBG_WARN("%d<%s,%d,%s>\n", cunt, judgment_list[cunt].key , judgment_list[cunt].value, judgment_list[cunt].pvalue);
    }
}
void judgment_list_init(void)
{
#ifdef PLATFORM_ID_0
    judgment_list[0].value = 0;
#endif
#ifdef PLATFORM_ID_1
    judgment_list[0].value = 1;
#endif
#ifdef PLATFORM_ID_2
    judgment_list[0].value = 2 ;
#endif
#ifdef PLATFORM_ID_3
    judgment_list[0].value = 3 ;
#endif
#ifdef PLATFORM_ID_4
    judgment_list[0].value = 4 ;
#endif
#ifdef PLATFORM_ID_5
    judgment_list[0].value = 5 ;
#endif
#ifdef PLATFORM_ID_6
    judgment_list[0].value = 6;
#endif
#ifdef PLATFORM_ID_7
    judgment_list[0].value = 7;
#endif
#ifdef PLATFORM_ID_8
    judgment_list[0].value = 8;
#endif
#ifdef PLATFORM_ID_9
    judgment_list[0].value = 9;
#endif
#ifdef PLATFORM_ID_10
    judgment_list[0].value = 10;
#endif
#ifdef PLATFORM_ID_11
    judgment_list[0].value = 11;
#endif
#ifdef PLATFORM_ID_12
    judgment_list[0].value = 12;
#endif
#ifdef PLATFORM_ID_13
    judgment_list[0].value = 13;
#endif
#ifdef PLATFORM_ID_14
    judgment_list[0].value = 14;
#endif
#ifdef PLATFORM_ID_15
    judgment_list[0].value = 15 ;
#endif
#ifdef PLATFORM_ID_16
    judgment_list[0].value = 16 ;
#endif
#ifdef PLATFORM_ID_17
    judgment_list[0].value = 17 ;
#endif
#ifdef PLATFORM_ID_18
    judgment_list[0].value = 18 ;
#endif
#ifdef PLATFORM_ID_19
    judgment_list[0].value = 19 ;
#endif
#ifdef PLATFORM_ID_20
    judgment_list[0].value = 20 ;
#endif
    judgment_list[1].value = is_out_updated ;
    judgment_list[2].value = gboot_mode ;
    judgment_list[3].value = g_recovery_meg->bootParam.upName.val ;
    {
        char temp_cmd[256];
        sprintf(temp_cmd, "setprop persist.lidbg.intPlatformId %d", judgment_list[0].value);
        lidbg_shell_cmd(temp_cmd);
    }
    g_var.platformid = judgment_list[0].value;
    LIDBG_WARN("g_var.platformid:%d\n", g_var.platformid);
    judgment_list[4].pvalue = g_var.car_type ;


    judgment_list_print();
}
struct judgment *get_judgment_list(char *key)
{
    int cunt, size;
    size = ARRAY_SIZE(judgment_list);
    for(cunt = 0; cunt < size; cunt++)
    {
        if(strncmp(key, judgment_list[cunt].key, strlen(judgment_list[cunt].key)) == 0)
            return  &judgment_list[cunt];
    }
    return NULL;
}

bool judgment_cmd(char *judgmentcmd, int *judgmenttimes)
{
    //example: ?PLFID=1||Factory=1
    //{
    //code...
    //}

    if( *(judgmentcmd) == '{' || *(judgmentcmd) == '}' || *judgmenttimes > 0)
    {
        if(*(judgmentcmd) == '}' )
            (*judgmenttimes) = 0;
        if(*(judgmentcmd) != '{' && *(judgmentcmd) != '}' )
            LIDBG_WARN("judgmenttimes = %d <skip.%s>\n", *judgmenttimes, judgmentcmd);
        goto skip_current_cmd;
    };
    // 2:check the cmd
    if(*(judgmentcmd) == '?')
    {
        char * or , * and ;
        //LIDBG_WARN("valid cmd======= <%s>============\n",  judgmentcmd);
        lidbgstrtrim(judgmentcmd);
        //LIDBG_WARN("after lidbgstrtrim <%s>\n", judgmentcmd);
        judgmentcmd += 1; //skip '?'
        or = strstr(judgmentcmd, "||");
        and = strstr(judgmentcmd, "&&");
        if( or != NULL && and != NULL)
        {
            LIDBG_WARN("or ,and == NULL <%s,%d,%d>\n",  judgmentcmd, or == NULL, and == NULL);
            goto skip_current_cmd;
        }

        lidbg_strrpl(judgmentcmd, or != NULL ? "||" : "&&", or != NULL ? "|" : "&");
        LIDBG_WARN("after lidbg_strrpl <%s>\n",  judgmentcmd);

        //toke judgment string and have a check if it is true
        {
            int loops, tokenvalue, judgmentvalue;
            char *arg[8] = {NULL};
            char *arg2[8] = {NULL};
            int cmd_num = lidbg_token_string(judgmentcmd, or != NULL ? "|" : "&" , arg) ;
            for(loops = 0; loops < cmd_num; loops++)
            {
                //LIDBG_WARN("%d/%d.start toke:<%s>\n", loops, cmd_num, arg[loops]);
                if(arg[loops] && lidbg_token_string(arg[loops], "=", arg2) == 2)
                {
                    struct judgment *p = get_judgment_list(arg2[0]);
                    if(p == NULL)
                    {
                        LIDBG_WARN("error unknown judgment <%s>\n", arg2[0]);
                        goto skip_current_cmd;
                    }
                    judgmentvalue = p->value;
                    tokenvalue = simple_strtoul(arg2[1], 0, 0);

                    LIDBG_WARN("<%d/%d. check :[%s,%d,%d]>\n", loops, cmd_num, arg2[0], tokenvalue, judgmentvalue);

                    //check
                    if( or != NULL || ( or == NULL && and == NULL))
                    {
                        if(p->pvalue == NULL)
                        {
                            //compare int
                            if( tokenvalue == judgmentvalue)
                            {
                                LIDBG_WARN("<judgment OK [||]>\n");
                                goto skip_current_cmd;
                            }
                        }
                        else
                        {
                            //compare String
                            LIDBG_WARN("<%d[||]judgmentString:[%s,%s] >\n", loops, p->pvalue, arg2[1]);
                            if (!strncmp(p->pvalue, arg2[1], strlen(arg2[1])) )
                            {
                                LIDBG_WARN("<judgment OK [||]>\n");
                                goto skip_current_cmd;
                            }
                        }

                        if(loops == cmd_num - 1)
                        {
                            LIDBG_WARN("<judgment error [||]>\n");
                            goto start_skip_below_cmd;
                        }
                    }

                    if( and != NULL)
                    {
                        if(p->pvalue == NULL)
                        {
                            //compare int
                            if( tokenvalue != judgmentvalue)
                            {
                                LIDBG_WARN("<judgment error [&&]>\n");
                                goto start_skip_below_cmd;
                            }
                        }
                        else
                        {
                            //compare String
                            LIDBG_WARN("<%d[&&]judgmentString:[%s,%s] >\n", loops, p->pvalue, arg2[1]);
                            if (strncmp(p->pvalue, arg2[1], strlen(arg2[1])) )
                            {
                                LIDBG_WARN("<judgment error [&&]>\n");
                                goto start_skip_below_cmd;
                            }
                        }

                        if(loops == cmd_num - 1)
                        {
                            LIDBG_WARN("<judgment OK [&&]>\n");
                            goto skip_current_cmd;
                        }
                    }
                }
                else if(arg[loops] != NULL)
                {
                    LIDBG_WARN("toke judgment string  err<%s>\n", arg[loops]);
                    goto skip_current_cmd;
                }
            }
            goto skip_current_cmd;
        }
    }
    else
        goto not_volid_judgment_cmd;

start_skip_below_cmd:
    //initrc's cmd below current cmd will be skipped until meet the cmd '}'
    *judgmenttimes = 1;
skip_current_cmd:
    //current cmd have sothing err, do not exe it and just skip.
    return true;
not_volid_judgment_cmd:
    //current cmd is a commen cmd not a judgment cmd likes "?platform_id=11".
    return false;
}

bool analyze_list_cmd(struct list_head *client_list)
{
    struct string_dev *pos;
    char *cmd[8] = {NULL};
    int cmd_num  = 0, tsleep = 0, judgmenttimes = 0;

    if (list_empty(client_list))
    {
        LIDBG_ERR("<list_is_empty>\n");
        return false;
    }

    list_for_each_entry(pos, client_list, tmp_list)
    {
        if(pos->yourkey)
        {
            if(judgment_cmd(pos->yourkey, &judgmenttimes) )
                continue;
            if(strlen(pos->yourkey) < 3)
                goto drop;
            if(strncmp(pos->yourkey, "msleep", sizeof("msleep") - 1) == 0)
            {
                cmd_num = lidbg_token_string(pos->yourkey, " ", cmd) ;
                if(cmd_num == 2)
                {
                    tsleep = simple_strtoul(cmd[1], 0, 0);
                    msleep(tsleep);
                    LIDBG_WARN("msleep<%d,%d>\n", cmd_num, tsleep);
                }
                else
                    goto drop;
            }
#ifdef SOC_msm8x25
            else if(strncmp(pos->yourkey, "cp", sizeof("cp") - 1) == 0)
            {
                cmd_num = lidbg_token_string(pos->yourkey, " ", cmd) ;
                if(cmd_num == 3)
                {
                    LIDBG_WARN("cp <%s,%s>\n", cmd[1], cmd[2]);
                    fs_copy_file(cmd[1], cmd[2]);
                }
                else
                    goto drop;
            }
#endif
            else
            {
                //msleep(100);
                //LIDBG_WARN("exe<%s>\n", pos->yourkey);
                lidbg_shell_cmd(pos->yourkey);
            }

            continue;
drop:
            LIDBG_WARN("bad cmd<%s>\n", pos->yourkey);
        }
    }

    return true;
}

static int thread_drivers_loader_analyze(void *data)
{
    char buff[50] = {0};
    judgment_list_init();


    fs_fill_list(get_lidbg_file_path(buff, "lidbg.init.rc.conf"), FS_CMD_FILE_LISTMODE, &lidbg_list);
    analyze_list_cmd(&lidbg_list);

    if((gboot_mode == MD_FLYSYSTEM) || (gboot_mode == MD_DEBUG))
    {
        LIDBG_WARN("<==gboot_mode==%d==>\n", gboot_mode);

        if(fs_is_file_exist("/flysystem/lib/modules/flyaudio.modules.conf"))
            fs_fill_list( "/flysystem/lib/modules/flyaudio.modules.conf", FS_CMD_FILE_LISTMODE, &flyaudio_hal_list);
        else
            fs_fill_list(get_lidbg_file_path(buff, "flyaudio.modules.conf"), FS_CMD_FILE_LISTMODE, &flyaudio_hal_list);

        analyze_list_cmd(&flyaudio_hal_list);

    }

    while(0 == g_var.android_boot_completed)
    {
        LIDBG_WARN("<flyaudio.init.rc.conf: waiting for g_var.android_boot_completed....>\n");
        ssleep(1);
    };

    fs_fill_list(get_lidbg_file_path(buff, "flyaudio.init.rc.conf"), FS_CMD_FILE_LISTMODE, &flyaudio_list);
    analyze_list_cmd(&flyaudio_list);


    ssleep(30);//later,exit
    return 0;
}
static int __init lidbg_drivers_loader_init(void)
{

    DUMP_FUN;
    LIDBG_GET;
    LIDBG_WARN("<==IN==>\n");
    CREATE_KTHREAD(thread_drivers_loader_analyze, NULL);
    LIDBG_MODULE_LOG;
    LIDBG_WARN("<==OUT==>\n\n");
    return 0;
}
static void __exit lidbg_drivers_loader_exit(void)
{
}

module_init(lidbg_drivers_loader_init);
module_exit(lidbg_drivers_loader_exit);

MODULE_AUTHOR("futengfei");
MODULE_DESCRIPTION("for hal or other group to insmod their KO or something else,2014.5.12");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(analyze_list_cmd);


