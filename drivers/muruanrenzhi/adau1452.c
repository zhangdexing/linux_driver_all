#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/fsl_devices.h>

#define ADAU1452_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
	SNDRV_PCM_FMTBIT_S32_LE)

static int adau1452_hw_params(struct snd_pcm_substream *substream,
                            struct snd_pcm_hw_params *params,
                            struct snd_soc_dai *dai)
{
//    struct snd_soc_pcm_runtime *rtd = substream->private_data;
//    struct snd_soc_codec *codec = rtd->codec;
//    struct wm8960_priv *wm8960 = snd_soc_codec_get_drvdata(codec);
//    u16 iface = snd_soc_read(codec, WM8960_IFACE1) & 0xfff3;
//    int i;
//
//    /* bit size */
//    switch (params_format(params)) {
//        case SNDRV_PCM_FORMAT_S16_LE:
//            break;
//        case SNDRV_PCM_FORMAT_S20_3LE:
//            iface |= 0x0004;
//            break;
//        case SNDRV_PCM_FORMAT_S24_LE:
//            iface |= 0x0008;
//            break;
//        default:
//            pr_err("wm8960 : SNDDRV PCM Format not Support! \n");
//            break;
//    }
//
//    /* Update filters for the new rate */
//    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
//        wm8960->playback_fs = params_rate(params);
//        wm8960_set_deemph(codec);
//    } else {
//        for (i = 0; i < ARRAY_SIZE(alc_rates); i++)
//            if (alc_rates[i].rate == params_rate(params))
//                snd_soc_update_bits(codec,
//                                    WM8960_ADDCTL3, 0x7,
//                                    alc_rates[i].val);
//    }
//
//    /* Update 3D Enhance */
//    if (params_rate(params) < 32000)
//        snd_soc_update_bits(codec, WM8960_3D, 0x60, 0x60);
//
//    /* set iface */
//    snd_soc_write(codec, WM8960_IFACE1, iface);
    return 0;
}

static int adau1452_mute(struct snd_soc_dai *dai, int mute)
{
//    struct snd_soc_codec *codec = dai->codec;
//    u16 mute_reg = snd_soc_read(codec, WM8960_DACCTL1) & 0xfff7;
//
//    if (mute)
//        snd_soc_write(codec, WM8960_DACCTL1, mute_reg | 0x8);
//    else
//        snd_soc_write(codec, WM8960_DACCTL1, mute_reg);
    return 0;
}

static int adau1452_set_dai_fmt(struct snd_soc_dai *codec_dai,
                              unsigned int fmt)
{
    struct snd_soc_codec *codec = codec_dai->codec;

    return 0;
}

static int adau1452_set_dai_clkdiv(struct snd_soc_dai *codec_dai,
                                 int div_id, int div)
{
    struct snd_soc_codec *codec = codec_dai->codec;

    return 0;
}

static int adau1452_set_dai_pll(struct snd_soc_dai *codec_dai, int pll_id,
                              int source, unsigned int freq_in, unsigned int freq_out) {
    struct snd_soc_codec *codec = codec_dai->codec;

    return 0;
}

static struct snd_soc_dai_ops adau1452_dai_ops = {
        .hw_params = adau1452_hw_params,
        .digital_mute = adau1452_mute,
        .set_fmt = adau1452_set_dai_fmt,
        .set_clkdiv = adau1452_set_dai_clkdiv,
        .set_pll = adau1452_set_dai_pll,
};

static struct snd_soc_dai_driver adau1452_dai = {
        .name = "adau1452-hifi",
        .playback = {
                .stream_name = "Playback",
                .channels_min = 1,
                .channels_max = 4,
                .rates = SNDRV_PCM_RATE_8000_96000,
                .formats = ADAU1452_FORMATS,},
        .capture = {
                .stream_name = "Capture",
                .channels_min = 1,
                .channels_max = 4,
                .rates = SNDRV_PCM_RATE_8000_96000,
                .formats = ADAU1452_FORMATS,},
        .ops = &adau1452_dai_ops,
        .symmetric_rates = 1,
};

static int adau1452_probe(struct snd_soc_codec *codec)
{
    printk("sandro add in %s %s @%d\n", __FILE__, __func__, __LINE__);
    return 0;
}
static int adau1452_remove(struct snd_soc_codec *codec)
{
//    struct wm8960_priv *wm8960 = snd_soc_codec_get_drvdata(codec);
//
//    wm8960->set_bias_level(codec, SND_SOC_BIAS_OFF);
    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_adau1452 = {
        .probe =    adau1452_probe,
        .remove =   adau1452_remove,
        //.suspend =  wm8960_suspend,
        //.resume =   wm8960_resume,
        //.set_bias_level = wm8960_set_bias_level,
        //.reg_cache_size = ARRAY_SIZE(wm8960_reg),
        //.reg_word_size = sizeof(u16),
        //.reg_cache_default = wm8960_reg,
};

static __devinit int codec_adau1452_probe(struct platform_device *dev)
{
//    struct wm8960_priv *wm8960;
    int ret;

//    wm8960 = kzalloc(sizeof(struct wm8960_priv), GFP_KERNEL);
//    if (wm8960 == NULL)
//        return -ENOMEM;
//
//    i2c_set_clientdata(i2c, wm8960);
//    wm8960->control_type = SND_SOC_I2C;
//    wm8960->control_data = i2c;
    printk("sandro add in %s %s @ %d\n", __FILE__, __func__, __LINE__);

    ret = snd_soc_register_codec(&dev->dev,
                                 &soc_codec_dev_adau1452, &adau1452_dai, 1);
//    if (ret < 0)
//        kfree(wm8960);
    return ret;
}

static int codec_adau1452_remove(struct platform_device *dev)
{
    return 0;
}


struct adau1452_data {
    bool capless;  /* Headphone outputs configured in capless mode */

    int dres;  /* Discharge resistance for headphone outputs */
    int gpio_base;
    //u32 gpio_init[WM8960_MAX_GPIO];
    u32 gpio_init[6];
    u32 mic_cfg;

    bool irq_active_low;

    bool spk_mono;   // Speaker o
};


static struct adau1452_data adau1452_config_data = {
        .gpio_init = {
        },
};

static struct platform_driver codec_adau1452_driver = {
        .probe = codec_adau1452_probe,
        .remove = codec_adau1452_remove,
        .driver = {
                .name = "adau1452-codec",
                .owner = THIS_MODULE,
        },
};

static struct platform_device codec_adau1452_device = {
        .name    = "adau1452-codec",
        //.id      = 0,
        .dev     =
                {
                        .platform_data = &adau1452_config_data,
                },
};

static int __init adau1452_modinit(void)
{
    int ret = 0;

    ret = platform_device_register(&codec_adau1452_device);
    if(ret)
    {
        printk(KERN_ERR "%s:%d: Can't register platform device %d\n", __FUNCTION__,__LINE__, ret);
        goto fail_reg_plat_dev;
    }

    ret = platform_driver_register(&codec_adau1452_driver);
    if(ret)
    {
        printk(KERN_ERR "%s:%d: Can't register platform driver %d\n", __FUNCTION__,__LINE__, ret);
        goto fail_reg_plat_drv;
    }

    return ret;

fail_reg_plat_drv:
    platform_driver_unregister(&codec_adau1452_driver);
fail_reg_plat_dev:
    return ret;
}
module_init(adau1452_modinit);

static void __exit adau1452_exit(void)
{

}
module_exit(adau1452_exit);

MODULE_DESCRIPTION("ASoC ADAU1452 codec driver");
MODULE_AUTHOR("Jimmy Chan");
MODULE_LICENSE("GPL");



