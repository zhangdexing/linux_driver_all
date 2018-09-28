# $(1): output file
define build-recoveryimage-target
  @echo ----- Making recovery image ------
  $(hide) mkdir -p $(TARGET_RECOVERY_OUT)
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/lib64/
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/etc $(TARGET_RECOVERY_ROOT_OUT)/tmp
  @echo Copying baseline ramdisk...
  $(hide) rsync -a $(TARGET_ROOT_OUT) $(TARGET_RECOVERY_OUT) # "cp -Rf" fails to overwrite broken symlinks on Mac.
  @echo Modifying ramdisk contents...
  $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/init*.rc
  $(if $(filter true,$(TARGET_USERIMAGES_USE_UBIFS)), \
    $(hide) cp -f $(recovery_ubiformat) $(TARGET_RECOVERY_ROOT_OUT)/sbin/ubiformat)
  $(hide) cp -f $(recovery_initrc) $(TARGET_RECOVERY_ROOT_OUT)/
  $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/sepolicy
  $(hide) cp -f $(recovery_sepolicy) $(TARGET_RECOVERY_ROOT_OUT)/sepolicy
  $(hide) cp $(TARGET_ROOT_OUT)/init.recovery.*.rc $(TARGET_RECOVERY_ROOT_OUT)/ || true # Ignore error when the src file doesn't exist.
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/res
  $(hide) rm -rf $(TARGET_RECOVERY_ROOT_OUT)/res/*
  $(hide) cp -rf $(recovery_resources_common)/* $(TARGET_RECOVERY_ROOT_OUT)/res
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/bin/ $(TARGET_RECOVERY_ROOT_OUT)/system/lib/modules/
  $(hide) cp -f $(toolbox_binary) $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -f $(toybox_binary) $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -f $(mksh_binary) $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -f $(linker_binary) $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -f $(library_binary) $(TARGET_RECOVERY_ROOT_OUT)/system/lib64/
  $(hide) cp -a $(PRODUCT_OUT)/obj/toolbox_links/* $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -a $(PRODUCT_OUT)/obj/toybox_links/* $(TARGET_RECOVERY_ROOT_OUT)/system/bin/
  $(hide) cp -rf $(call include-path-for, recovery)/flyRecovery/out/ \
                                       $(TARGET_RECOVERY_ROOT_OUT)/system/lib/modules/
  $(hide) cp -rf $(call include-path-for, recovery)/flyRecovery/out/lidbg_load \
                                       $(TARGET_RECOVERY_ROOT_OUT)/sbin/
  $(hide) cp -rf $(call include-path-for, recovery)/flyRecovery/font/ $(TARGET_RECOVERY_ROOT_OUT)/res/

  $(hide) cp -f $(recovery_font) $(TARGET_RECOVERY_ROOT_OUT)/res/images/font.png
  $(if $(filter yes,$(TRUSTONIC_TEE_SUPPORT)), \

    $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/system/bin/mcDriverDaemon
    $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/bin; cp -f $(call intermediates-dir-for,EXECUTABLES,mcDriverDaemon)/mcDriverDaemon $(TARGET_RECOVERY_ROOT_OUT)/system/bin
    $(hide) rm -fr $(TARGET_RECOVERY_ROOT_OUT)/system/app/mcRegistry
    $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/app/mcRegistry; cp -f $(shell find $(PRODUCT_OUT)/system/app/mcRegistry -type f -name "*.drbin" -print) $(TARGET_RECOVERY_ROOT_OUT)/system/app/mcRegistry
    $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/app/mcRegistry; cp -f $(shell find $(PRODUCT_OUT)/system/app/mcRegistry -type f -name "*.tlbin" -print) $(TARGET_RECOVERY_ROOT_OUT)/system/app/mcRegistry)
  $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/sbin/meta_tst
  $(hide) $(foreach item,$(recovery_resources_private), \
    cp -rf $(item) $(TARGET_RECOVERY_ROOT_OUT)/)
  $(hide) $(foreach item,$(recovery_fstab), \
    cp -f $(item) $(TARGET_RECOVERY_ROOT_OUT)/etc/recovery.fstab)
  $(hide) cp -f $(RECOVERY_INSTALL_OTA_KEYS) $(TARGET_RECOVERY_ROOT_OUT)/res/keys
  $(hide) cat $(INSTALLED_DEFAULT_PROP_TARGET) $(recovery_build_prop) \
          > $(TARGET_RECOVERY_ROOT_OUT)/default.prop
  $(hide) $(MKBOOTFS) -d $(TARGET_OUT) $(TARGET_RECOVERY_ROOT_OUT) | $(MINIGZIP) > $(recovery_ramdisk)
  $(if $(filter yes,$(MTK_HEADER_SUPPORT)), \
    $(hide) cp -f $(recovery_ramdisk) $(recovery_ramdisk_bthdr)
    $(hide) $(HOST_OUT_EXECUTABLES)/mkimage $(recovery_ramdisk_bthdr) ROOTFS 0xffffffff > $(PRODUCT_OUT)/ramdisk_recovery_bthdr.img
    $(hide) mv $(PRODUCT_OUT)/ramdisk_recovery_bthdr.img $(recovery_ramdisk_bthdr)
    $(hide) $(HOST_OUT_EXECUTABLES)/mkimage $(recovery_ramdisk) RECOVERY 0xffffffff > $(PRODUCT_OUT)/ramdisk_recovery.img
    $(hide) mv $(PRODUCT_OUT)/ramdisk_recovery.img $(recovery_ramdisk))
  $(if $(filter true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_VBOOT)), \
    $(hide) $(MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $(1).unsigned, \
    $(hide) $(MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $(1) --id > $(RECOVERYIMAGE_ID_FILE))
  $(if $(filter true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_BOOT_SIGNER)),\
    $(BOOT_SIGNER) /recovery $(1) $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).pk8 $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).x509.pem $(1))
  $(if $(filter true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_VBOOT)), \
    $(VBOOT_SIGNER) $(FUTILITY) $(1).unsigned $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VBOOT_SIGNING_KEY).vbpubk $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VBOOT_SIGNING_KEY).vbprivk $(1).keyblock $(1))
  $(hide) $(call assert-max-image-size,$(1),$(BOARD_RECOVERYIMAGE_PARTITION_SIZE))
  $(if $(filter yes,$(MTK_HEADER_SUPPORT)), \
    $(hide) $(MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_BTHDR_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $(PRODUCT_OUT)/recovery_bthdr.img
    $(hide) $(call assert-max-image-size,$(PRODUCT_OUT)/recovery_bthdr.img,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE)))
  @echo ----- Made recovery image: $(1) --------
endef
