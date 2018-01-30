#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/fsl_devices.h>
//#include <mach/audmux.h>


//struct imx_priv {
//    int sysclk;         /*mclk from the outside*/
//    int codec_sysclk;
//    int dai_hifi;
//    int hp_irq;
//    int hp_status;
//    int amic_irq;
//    int amic_status;
//    struct platform_device *pdev;
//    struct switch_dev sdev;
//    struct snd_pcm_substream *first_stream;
//    struct snd_pcm_substream *second_stream;
//};
//
//static struct imx_priv card_priv;

//struct mxc_audio_platform_data {
//    int ssi_num;
//    int src_port;
//    int ext_port;
//
//    int intr_id_hp;
//    int ext_ram;
//    struct clk *ssi_clk[2];
//
//    int hp_gpio;
//    int hp_active_low;  /* headphone irq is active low */
//
//    int mic_gpio;
//    int mic_active_low; /* micphone irq is active low */
//
//    int sysclk;
//    const char *codec_name;
//
//    int (*init) (void); /* board specific init */
//    int (*amp_enable) (int enable);
//    int (*clock_enable) (int enable);
//    int (*finit) (void);    /* board specific finit */
//    void *priv;     /* used by board specific functions */
//};

static struct snd_soc_card snd_soc_card_imx;

/*
static int imx_audmux_config(int slave, int master)
{
    unsigned int ptcr, pdcr;
    slave = slave - 1;
    master = master - 1;

    ptcr = MXC_AUDMUX_V2_PTCR_SYN |
           MXC_AUDMUX_V2_PTCR_TFSDIR |
           MXC_AUDMUX_V2_PTCR_TFSEL(master) |
           MXC_AUDMUX_V2_PTCR_TCLKDIR |
           MXC_AUDMUX_V2_PTCR_TCSEL(master);
    pdcr = MXC_AUDMUX_V2_PDCR_RXDSEL(master);
    mxc_audmux_v2_configure_port(slave, ptcr, pdcr);

    ptcr = MXC_AUDMUX_V2_PTCR_SYN;
    pdcr = MXC_AUDMUX_V2_PDCR_RXDSEL(slave);
    mxc_audmux_v2_configure_port(master, ptcr, pdcr);

    return 0;
}
*/

static int  imx_adau1452_probe(struct platform_device *pdev)
{
    struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
    //struct imx_priv *priv = &card_priv;
    int ret = 0;

    //priv->pdev = pdev;
//    printk("sandro add in %s %d plat->src_port=%d plat->ext_port=%d\n", __func__, __LINE__, plat->src_port, plat->ext_port);
    printk("sandro add in %s %d \n", __func__, __LINE__);
//    imx_audmux_config(plat->src_port, plat->ext_port);
//
//    if (plat->init && plat->init()) {
//        ret = -EINVAL;
//        return ret;
//    }
//
//
//
//    priv->sysclk = plat->sysclk;
//    priv->hp_gpio = plat->hp_gpio;
//    priv->hp_active_low = plat->hp_active_low;
//    return ret;
    return 0;
}

static int imx_adau1452_remove(struct platform_device *pdev)
{

    return 0;
}

static int imx_adau1452_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

static int imx_hifi_hw_params(struct snd_pcm_substream *substream,
                              struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    //struct imx_priv *priv = &card_priv;
    unsigned int channels = params_channels(params);
    unsigned int rate = params_rate(params);
//    int bclk = snd_soc_params_to_bclk(params);
    int i,ret=0;
    u32 dai_format;

//    if (channels == 1)
//        bclk *= 2;

    printk("sandro add in %s %d channels=%u\n", __func__, __LINE__, channels);
    dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM ;
    //dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS ;

    /* set codec DAI configuration */
//    ret = snd_soc_dai_set_fmt(codec_dai, dai_format);
//    if (ret < 0){
//        pr_err("-%s(): Cannot set codec_dai format \n", __FUNCTION__);
//        return -EINVAL;
//    }

    /* set i.MX active slot mask */
    snd_soc_dai_set_tdm_slot(cpu_dai,
                             channels == 1 ? 0xfffffffe : 0xfffffffc,
                             channels == 1 ? 0xfffffffe : 0xfffffffc,
                             2, 32);

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai, dai_format);
    if (ret < 0){
        pr_err("-%s(): Cannot set cpu_dai format \n", __FUNCTION__);
        return -EINVAL;
    }


//    /* find the sysclk rate && adc/dac div */
//    for (i=0; i < ARRAY_SIZE(adac_divs); i++) {
//        if (adac_divs[i].rate == rate) {
//            sysclk = adac_divs[i].sysclk;
//            snd_soc_dai_set_clkdiv(codec_dai, WM8960_ADCDIV, adac_divs[i].div);
//            snd_soc_dai_set_clkdiv(codec_dai, WM8960_DACDIV, adac_divs[i].div);
//            break;
//        }
//    }
//
//    if (i == ARRAY_SIZE(adac_divs)) {
//        pr_err("Rate %dHz is not support.\n",rate);
//        return -EINVAL;
//    }
//
//    /* find the bclk div */
//    for (i=0; i < ARRAY_SIZE(bclk_divs); i++) {
//        if (bclk_divs[i] > 0) {
//            if (sysclk/bclk_divs[i] == bclk) {
//                snd_soc_dai_set_clkdiv(codec_dai, WM8960_BCLKDIV, i);
//                break;
//            }
//        }
//    }
//
//    if (i == ARRAY_SIZE(bclk_divs)) {
//        pr_err("Unsupported BCLK ratio %d\n", sysclk/bclk);
//    }
//
//    ret = snd_soc_dai_set_pll(codec_dai, 0, 0, priv->sysclk, sysclk);
//    if( ret < 0 ){
//        pr_err("-%s(): Codec PLL setting error, %d\n", __FUNCTION__, ret);
//        return ret;
//    }
    printk("sandro add in %s %d\n", __func__, __LINE__);
    return ret;
}

static int imx_hifi_startup(struct snd_pcm_substream *substream)
{
//    struct snd_soc_pcm_runtime *rtd = substream->private_data;
//    struct snd_soc_dai *codec_dai = rtd->codec_dai;
//    struct imx_priv *priv = &card_priv;
//    struct mxc_audio_platform_data *plat = priv->pdev->dev.platform_data;
//
//    if (!codec_dai->active)
//        plat->clock_enable(1);

    return 0;
}

static void imx_hifi_shutdown(struct snd_pcm_substream *substream)
{
//    struct snd_soc_pcm_runtime *rtd = substream->private_data;
//    struct snd_soc_dai *codec_dai = rtd->codec_dai;
//    struct imx_priv *priv = &card_priv;
//    struct mxc_audio_platform_data *plat = priv->pdev->dev.platform_data;
//
//    if (!codec_dai->active)
//        plat->clock_enable(0);

}


/*
static struct snd_soc_dai_driver adau_dai = {
        .name = "adau1452",
        .playback = {
                .stream_name = "Playback",
                .channels_min = 1,
                .channels_max = 2,
                .rates = ADAU1452_RATES,
                .formats = ADAU1452_FORMATS,},
        .capture = {
                .stream_name = "Capture",
                .channels_min = 1,
                .channels_max = 2,
                .rates = ADAU1452_RATES,
                .formats = ADAU1452_FORMATS,},
        .ops = &adau1452_dai_ops,
        .symmetric_rates = 1,
};*/


static struct snd_soc_ops imx_hifi_ops = {
        //.startup = imx_hifi_startup,
        //.shutdown = imx_hifi_shutdown,
        .hw_params = imx_hifi_hw_params,
};

static struct snd_soc_dai_link imx_dai[] = {
        {
                .name = "HiFi",
                .stream_name = "HiFi",
                .codec_dai_name = "adau1452-hifi",
                .codec_name = "adau1452-codec.0",
                .cpu_dai_name   = "mt-soc-dl1dai-driver",
                .platform_name  = "mt-soc-dl1-pcm",
                .init       = imx_adau1452_init,
                .ops        = &imx_hifi_ops,
        },
};

static struct snd_soc_card snd_soc_card_imx = {
        .name       = "adau1452-audio",
        .dai_link   = imx_dai,
        .num_links  = ARRAY_SIZE(imx_dai),
};

/*
static struct mxc_audio_platform_data adau1452_data =
        {
                .ssi_num = 1,
                .src_port = 2,
                .ext_port = 3,
                //.hp_gpio = TOPEET_HEADPHONE_DET,
                //.hp_active_low = 1,
                //.mic_gpio = TOPEET_MICROPHONE_DET,
                //.mic_active_low = 1,
                //.init = mxc_wm8960_init,
                //.clock_enable = wm8960_clk_enable,
        };
*/

static struct platform_device mx6_topeet_audio_adau1452_device =
        {
                .name = "imx-adau1452",
        };


static struct platform_driver adau1452_driver = {
        .probe = imx_adau1452_probe,
        .remove = imx_adau1452_remove,
        .driver = {
                .name = "imx-adau1452",
                .owner = THIS_MODULE,
        },
};


static int __init mxc_register_device(struct platform_device *pdev, void *data)
{
    int ret;

    pdev->dev.platform_data = data;

    ret = platform_device_register(pdev);
    if (ret)
        pr_debug("Unable to register platform device '%s': %d\n",
                 pdev->name, ret);

    return ret;
}

static struct platform_device *imx_snd_device;

static int __init imx_adau_asoc_init(void)
{
    int ret;

    printk("sandro add in %s %d\n", __func__, __LINE__);
    mxc_register_device(&mx6_topeet_audio_adau1452_device, NULL);
    ret = platform_driver_register(&adau1452_driver);
    if (ret < 0)
        goto exit;

    printk("sandro add in %s %d add soc-audio.5 device.\n", __func__, __LINE__);
    imx_snd_device = platform_device_alloc("soc-audio", 5);
    if (!imx_snd_device)
        goto err_device_alloc;

    platform_set_drvdata(imx_snd_device, &snd_soc_card_imx);



    ret = platform_device_add(imx_snd_device);

    if (0 == ret)
        goto exit;

    platform_device_put(imx_snd_device);

    err_device_alloc:
    platform_driver_unregister(&adau1452_driver);
    exit:
    printk("sandro add in %s %d\n", __func__, __LINE__);
    return ret;
}

static void  imx_adau_asoc_exit(void)
{
    platform_driver_unregister(&adau1452_driver);
    platform_device_unregister(imx_snd_device);
}

module_init(imx_adau_asoc_init);
module_exit(imx_adau_asoc_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC imx adau1452");
MODULE_LICENSE("GPL");
