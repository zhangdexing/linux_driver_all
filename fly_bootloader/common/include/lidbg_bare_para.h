#ifndef _LIGDBG_FLYBAREDATA__
#define _LIGDBG_FLYBAREDATA__

#define FLAG_VALID (0x12345678)
#define MEM_SIZE_512_KB              (0x00080000)


struct bare_info
{
    int uart_dbg_en;
    int reserve[31];
};
typedef struct
{
    int flag_valid;
    int flag_dirty;
    struct bare_info bare_info;
}fly_bare_data;

extern fly_bare_data *bare_data_get(char *who);
extern bool bare_data_write(char *who, fly_bare_data *p_info);
extern void bare_data_show(char *who, fly_bare_data *p_info);

#endif
