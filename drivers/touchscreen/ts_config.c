
#include "lidbg.h"

//+++++++++++++++++++++++++++++++++gt9xx+++++++++++++++++++++++++++++++++++++
#include "gt9xx.h"
LIDBG_DEFINE;

u8 cfg_info_group1_v1[] = CTP_CFG_GROUP1_V1;
u8 cfg_info_group2_v1[] = CTP_CFG_GROUP2_V1;
u8 cfg_info_group1[] = CTP_CFG_GROUP1;
u8 cfg_info_group2[] = CTP_CFG_GROUP2;
u8 cfg_info_group3[] = CTP_CFG_GROUP3;
u8 cfg_info_group4[] = CTP_CFG_GROUP4;
u8 cfg_info_group5[] = CTP_CFG_GROUP5;
u8 cfg_info_group6[] = CTP_CFG_GROUP6;
u8 cfg_info_group7[] = CTP927_CFG_GROUP1;
u8 cfg_info_group8[] = CTP927_CFG_GROUP2;
u8 cfg_info_group9[] = CTP927_CFG_GROUP3;
u8 cfg_info_group10[] = CTP927_CFG_GROUP4;
u8 cfg_info_group11[] = CTP927_CFG_GROUP5;
u8 cfg_info_group12[] = CTP927_CFG_GROUP6;
u8 cfg_info_group13[] = CTP_CFG_GROUP7;
u8 cfg_info_group14[] = CTP_CFG_GROUP8;
u8 cfg_info_group15[] = CTP_CFG_GROUP9;
u8 cfg_info_group16[] = CTP_CFG_GROUP10;
u8 cfg_info_group17[] = CTP_CFG_GROUP11;
u8 cfg_info_group18[] = CTP_CFG_GROUP12;
u8 cfg_info_group19[] = CTP_CFG_GROUP13;
u8 cfg_info_group20[] = CTP_CFG_GROUP14;
u8 cfg_info_group21[] = CTP_CFG_GROUP15;
u8 cfg_info_group22[] = CTP_CFG_GROUP16;
u8 cfg_info_group23[] = CTP_CFG_GROUP17;
u8 cfg_info_group24[] = CTP_CFG_GROUP18;
u8 cfg_info_group25[] = CTP_CFG_GROUP19;
u8 cfg_info_group26[] = CTP_CFG_GROUP20;
u8 cfg_info_group27[] = CTP_CFG_GROUP21;
u8 cfg_info_group28[] = CTP927_CFG_GROUP7;
u8 cfg_info_group29[] = CTP927_CFG_GROUP8;
u8 cfg_info_group30[] = CTP927_CFG_GROUP9;
u8 cfg_info_group31[] = CTP_CFG_GROUP22;
u8 cfg_info_group32[] = CTP_CFG_GROUP23;
u8 cfg_info_group33[] = CTP927_CFG_GROUP10;
u8 cfg_info_group34[] = CTP927_CFG_GROUP11;
u8 cfg_info_group35[] = CTP927_CFG_GROUP12;
u8 cfg_info_group36[] = CTP927_CFG_GROUP13;
u8 cfg_info_group37[] = CTP_CFG_GROUP14;
u8 cfg_info_group38[] = CTP927_CFG_GROUP14;
u8 cfg_info_group39[] = CTP_CFG_GROUP23;
u8 cfg_info_group40[] = CTP927_CFG_GROUP15;
u8 cfg_info_group41[] = CTP911_CFG_GROUP16;
u8 cfg_info_group42[] = CTP_CFG_GROUP24;
u8 cfg_info_group43[] = CTP927_CFG_GROUP16;
u8 cfg_info_group44[] = CTP928_CFG_GROUP16;
u8 cfg_info_group45[] = CTP927_CFG_GROUP17;
u8 cfg_info_group46[] = CTP911_CFG_GROUP17;
u8 cfg_info_group47[] = CTP911_CFG_GROUP18;
u8 cfg_info_group48[] = CTP928_CFG_GROUP17;
u8 cfg_info_group49[] = CTP927_CFG_GROUP18;
u8 cfg_info_group50[] = CTP_CFG_GROUP25;
u8 cfg_info_group51[] = CTP928_CFG_GROUP18;
u8 cfg_info_group52[] = CTP928_CFG_GROUP19;
u8 cfg_info_group53[] = CTP_CFG_GROUP26;
u8 cfg_info_group54[] = CTP927_CFG_GROUP16_1;

u8 *send_cfg_buf[] = {cfg_info_group1, cfg_info_group2,
                  cfg_info_group3, cfg_info_group4, cfg_info_group5, cfg_info_group6,
                  cfg_info_group7, cfg_info_group8, cfg_info_group9, cfg_info_group10, cfg_info_group11,
                  cfg_info_group12, cfg_info_group13, cfg_info_group14, cfg_info_group15, cfg_info_group16,
                  cfg_info_group17, cfg_info_group18, cfg_info_group19, cfg_info_group20, cfg_info_group21,
                  cfg_info_group22, cfg_info_group23, cfg_info_group24, cfg_info_group25, cfg_info_group26, cfg_info_group27,
                  cfg_info_group28, cfg_info_group29,cfg_info_group30,
		  cfg_info_group31, cfg_info_group32,cfg_info_group33,cfg_info_group34,cfg_info_group35,cfg_info_group36,
		  cfg_info_group37, cfg_info_group38,cfg_info_group39,cfg_info_group40,cfg_info_group41,cfg_info_group42,
			cfg_info_group43, cfg_info_group44, cfg_info_group45, cfg_info_group46, cfg_info_group47, cfg_info_group48, cfg_info_group49,
			cfg_info_group50, cfg_info_group51, cfg_info_group52, cfg_info_group53, cfg_info_group54
                 };
EXPORT_SYMBOL(send_cfg_buf);

u8 cfg_info_len[] = {CFG_GROUP_LEN(cfg_info_group1),
                 CFG_GROUP_LEN(cfg_info_group2),
                 CFG_GROUP_LEN(cfg_info_group3),
                 CFG_GROUP_LEN(cfg_info_group4),
                 CFG_GROUP_LEN(cfg_info_group5),
                 CFG_GROUP_LEN(cfg_info_group6),
                 CFG_GROUP_LEN(cfg_info_group7),
                 CFG_GROUP_LEN(cfg_info_group8),
                 CFG_GROUP_LEN(cfg_info_group9),
                 CFG_GROUP_LEN(cfg_info_group10),
                 CFG_GROUP_LEN(cfg_info_group11),
                 CFG_GROUP_LEN(cfg_info_group12),
                 CFG_GROUP_LEN(cfg_info_group13),
                 CFG_GROUP_LEN(cfg_info_group14),
                 CFG_GROUP_LEN(cfg_info_group15),
                 CFG_GROUP_LEN(cfg_info_group16),
                 CFG_GROUP_LEN(cfg_info_group17),
                 CFG_GROUP_LEN(cfg_info_group18),
                 CFG_GROUP_LEN(cfg_info_group19),
                 CFG_GROUP_LEN(cfg_info_group20),
                 CFG_GROUP_LEN(cfg_info_group21),
                 CFG_GROUP_LEN(cfg_info_group22),
                 CFG_GROUP_LEN(cfg_info_group23),
                 CFG_GROUP_LEN(cfg_info_group24),
                 CFG_GROUP_LEN(cfg_info_group25),
                 CFG_GROUP_LEN(cfg_info_group26),
                 CFG_GROUP_LEN(cfg_info_group27),
                 CFG_GROUP_LEN(cfg_info_group28),
                 CFG_GROUP_LEN(cfg_info_group29),
                 CFG_GROUP_LEN(cfg_info_group30),
		 CFG_GROUP_LEN(cfg_info_group31),
		 CFG_GROUP_LEN(cfg_info_group32),
		 CFG_GROUP_LEN(cfg_info_group33),
		 CFG_GROUP_LEN(cfg_info_group34),
		 CFG_GROUP_LEN(cfg_info_group35),
		 CFG_GROUP_LEN(cfg_info_group36),
		 CFG_GROUP_LEN(cfg_info_group37),
		 CFG_GROUP_LEN(cfg_info_group38),
		 CFG_GROUP_LEN(cfg_info_group39),
		 CFG_GROUP_LEN(cfg_info_group40),
		 CFG_GROUP_LEN(cfg_info_group41),
		 CFG_GROUP_LEN(cfg_info_group42),
		 CFG_GROUP_LEN(cfg_info_group43),
		 CFG_GROUP_LEN(cfg_info_group44),
		 CFG_GROUP_LEN(cfg_info_group45),
		 CFG_GROUP_LEN(cfg_info_group46),
		 CFG_GROUP_LEN(cfg_info_group47),
		 CFG_GROUP_LEN(cfg_info_group48),
		 CFG_GROUP_LEN(cfg_info_group49),
		 CFG_GROUP_LEN(cfg_info_group50),
		 CFG_GROUP_LEN(cfg_info_group51),
		 CFG_GROUP_LEN(cfg_info_group52),
		 CFG_GROUP_LEN(cfg_info_group53),
		 CFG_GROUP_LEN(cfg_info_group54),

                };

EXPORT_SYMBOL(cfg_info_len);
//-----------------------------------gt9xx-----------------------------------




static int ts_config_init(void)
{
    LIDBG_GET;
    DUMP_BUILD_TIME;
    if(0)
   {
    	g_var.hw_info.ts_config = 54;
    	g_var.hw_info.virtual_key = 0;
    }
    return 0;
}


module_init(ts_config_init);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("flyaudio");
