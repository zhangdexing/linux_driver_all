
#include "lidbg.h"

struct spi_api
{
    struct list_head list;
    struct spi_device *spi;
};

#define SPI_MINORS    32
#define SPI_MODE_MASK (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
        | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
        | SPI_NO_CS | SPI_READY)
#define SPIDEV_MAJOR			153	/* assigned */
static LIST_HEAD(spi_api_list);
static DEFINE_SPINLOCK(spi_api_list_lock);
struct spi_device *spi;
static struct spi_api *get_spi_api(int bus_id)
{
    struct spi_api *spi_api;

    spin_lock(&spi_api_list_lock);
    list_for_each_entry(spi_api, &spi_api_list, list)
    {
        if (spi_api->spi->master->bus_num == bus_id)
            goto found;
    }
    spi_api = NULL;

found:
    spin_unlock(&spi_api_list_lock);
    return spi_api;
}

static struct spi_api *add_spi_api(struct spi_device *spi)
{
    struct spi_api *spi_api;

    if (spi->master->bus_num >= SPI_MINORS)
    {
        lidbg(KERN_ERR "spi_api: Out of device minors (%d)\n",
              spi->master->bus_num);
        return NULL;
    }

    spi_api = kzalloc(sizeof(*spi_api), GFP_KERNEL);
    if (!spi_api)
        return NULL;
    spi_api->spi = spi;

    spin_lock(&spi_api_list_lock);
    list_add_tail(&spi_api->list, &spi_api_list);
    spin_unlock(&spi_api_list_lock);
    return spi_api;
}

static void del_spi_api(struct spi_api *spi_api)
{
    spin_lock(&spi_api_list_lock);
    list_del(&spi_api->list);
    spin_unlock(&spi_api_list_lock);
    kfree(spi_api);
}

int spi_api_do_set(int bus_id,
                   u8 mode,
                   u8 bits_per_word,
                   u32 max_speed_hz)
{
    u8 tmp = mode;
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;

    spi = spi_api->spi;
    //spi->mode &= ~SPI_MODE_MASK;
    //spi->mode &= SPI_MODE_MASK;

    tmp |= spi->mode & ~SPI_MODE_MASK;
    spi->mode = (u8)tmp;
    //spi->mode |= mode;
    spi->bits_per_word = bits_per_word;
    spi->max_speed_hz = max_speed_hz;
    return spi_setup(spi);
}

int spi_api_do_write(int bus_id, const u8 *buf, size_t len)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_write(spi_api->spi, buf, len);
}

int spi_api_do_read(int bus_id, u8 *buf, size_t len)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_read(spi_api->spi, buf, len);
}

int spi_api_do_write_then_read(int bus_id,
                               const u8 *txbuf, unsigned n_tx,
                               u8 *rxbuf, unsigned n_rx)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_write_then_read(spi_api->spi, txbuf, n_tx, rxbuf, n_rx);
}

static int spi_api_probe(struct spi_device *spi)
{
    add_spi_api(spi);
    lidbg(KERN_INFO "spi_api_probe device[%d]\n", spi->master->bus_num);
    return 0;
}

static int spi_api_remove(struct spi_device *spi)
{
    struct spi_api *spi_api = get_spi_api(spi->master->bus_num);
    if (spi_api)
        del_spi_api(spi_api);
    return 0;
}

static struct spi_driver spi_api_driver =
{
    .driver = {
        .name = "SPI-API",
        .owner = THIS_MODULE,
    },
    .probe = spi_api_probe,
    .remove = spi_api_remove,
};

static int __init spi_api_init(void)
{
    int status;
    struct spi_board_info chip =
    {
        .modalias	= "SPI-API",
        .mode = 0x01,
        .bus_num	= 0,
        .chip_select = 0,
        .max_speed_hz = 19200000,
    };

    struct spi_master *master;

    //struct spi_device *spi;
    status = spi_register_driver(&spi_api_driver);
    if (status < 0)
        return status;

    master = spi_busnum_to_master(0);
    if (!master)
    {
        status = -ENODEV;
        goto error_busnum;
    }
    spi = spi_new_device(master, &chip);
    if (!spi)
    {
        status = -EBUSY;
        goto error_mem;
    }
    if (status)
    {
        lidbg(KERN_ERR "[%s] Driver registration failed, module not inserted.\n", __func__);
        return status;
    }
    lidbg("spi_api_init\n");
    DUMP_BUILD_TIME;
    LIDBG_MODULE_LOG;
    return 0 ;
error_mem:
error_busnum:
    spi_unregister_driver(&spi_api_driver);
    return status;
}

static void __exit spi_api_exit(void)
{
    if (spi)
    {
        spi_unregister_device(spi);
        spi = NULL;
    }
    spi_unregister_driver(&spi_api_driver);
}


void mod_spi_main(int argc, char **argv)
{

    if(argc < 5)
    {
        lidbg("Usage:\n");
        lidbg("r bus_id dev_addr start_reg num\n");
        lidbg("w bus_id dev_addr num data1 data2 ...\n");
        return;

    }
    //...
    if(!strcmp(argv[0], "w"))
    {
        int bus_id,  num, i;
        char *psend_data;
        u32 mode, bits_per_word, max_speed_hz;
        bus_id = simple_strtoul(argv[1], 0, 0);
        num = simple_strtoul(argv[2], 0, 0);
        mode = simple_strtoul(argv[3], 0, 0);
        bits_per_word  = simple_strtoul(argv[4], 0, 0);
        max_speed_hz  = simple_strtoul(argv[5], 0, 0);
        lidbg("bus_id:%dnum:%dmode:%dbits_per_word:%dmax_speed_hz:%d\n", bus_id, num, mode, bits_per_word, max_speed_hz);
        if(argc - 6 < num)
        {
            lidbg("input num err:\n");
            return;
        }
        psend_data = (char *)kzalloc(num, GFP_KERNEL);
        for(i = 0; i < num; i++)
        {
            psend_data[i] = (char)simple_strtoul(argv[i + 6], 0, 0);
        }
        spi_api_do_set( bus_id, mode, bits_per_word, max_speed_hz);
        spi_api_do_write( bus_id, &psend_data[0], num);
        kfree(psend_data);
    }
    if(!strcmp(argv[0], "r"))
    {
        int bus_id,  num, i;
        char *precv_data;
        u32 mode, bits_per_word, max_speed_hz;
        bus_id = simple_strtoul(argv[1], 0, 0);
        num = simple_strtoul(argv[2], 0, 0);
        mode = simple_strtoul(argv[3], 0, 0);
        bits_per_word  = simple_strtoul(argv[4], 0, 0);
        max_speed_hz  = simple_strtoul(argv[5], 0, 0);
        if(argc - 6 < num)
        {
            lidbg("input num err:\n");
            return;
        }
        precv_data = (char *)kzalloc(num, GFP_KERNEL);
        for(i = 0; i < num; i++)
        {
            precv_data[i] = (char)simple_strtoul(argv[i + 6], 0, 0);
        }
        spi_api_do_set( bus_id, mode, bits_per_word, max_speed_hz);
        spi_api_do_read( bus_id, &precv_data[0], num);
        lidbg("====precv_datap:%d%d%d\n", precv_data[0] , precv_data[1] , precv_data[2]);
        kfree(precv_data);
    }
    if(!strcmp(argv[0], "wr"))
    {
        int bus_id,  num, i;
        char *psend_data, *precv_data;
        u32 mode, bits_per_word, max_speed_hz;
        bus_id = simple_strtoul(argv[1], 0, 0);
        num = simple_strtoul(argv[2], 0, 0);
        mode = simple_strtoul(argv[3], 0, 0);
        bits_per_word  = simple_strtoul(argv[4], 0, 0);
        max_speed_hz  = simple_strtoul(argv[5], 0, 0);
        if(argc - 6 < num)
        {
            lidbg("input num err:\n");
            return;
        }
        psend_data = (char *)kzalloc(num, GFP_KERNEL);
        precv_data = (char *)kzalloc(num, GFP_KERNEL);
        for(i = 0; i < num; i++)
        {
            psend_data[i] = (char)simple_strtoul(argv[i + 6], 0, 0);
        }
        psend_data = (char *)kzalloc(num, GFP_KERNEL);
        for(i = 0; i < num; i++)
        {
            precv_data[i] = (char)simple_strtoul(argv[i + num + 6], 0, 0);
        }
        spi_api_do_set( bus_id, mode, bits_per_word, max_speed_hz);
        spi_api_do_write_then_read(bus_id, &psend_data[0], num, &precv_data[0], num);
        lidbg("psend_datap:%d\n", psend_data[0]);
        lidbg("====precv_datap:%d%d%d\n", precv_data[0] , precv_data[1] , precv_data[2]);
        kfree(psend_data);
        kfree(precv_data);
    }
}


MODULE_AUTHOR("Loon, <sepnic@gmail.com>");
MODULE_DESCRIPTION("SPI spi_api Driver");
MODULE_LICENSE("GPL");

module_init(spi_api_init);
module_exit(spi_api_exit);

EXPORT_SYMBOL(spi_api_do_set);
EXPORT_SYMBOL(spi_api_do_write);
EXPORT_SYMBOL(spi_api_do_read);
EXPORT_SYMBOL(spi_api_do_write_then_read);
EXPORT_SYMBOL(mod_spi_main);


