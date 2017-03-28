
#SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,FastPowerOn)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,LidbgCommenLogic)
ifeq ($(CONFIG_APP_LIDBGPMSERVICE),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,LidbgPmService)
else
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,FlyBootService)
endif
#SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,SleepTest)
include $(SUBDIR_MAKEFILES)
