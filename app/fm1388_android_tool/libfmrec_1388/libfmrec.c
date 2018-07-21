/*
 * external/libfmrec_1388/libfmrec.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * convert pcm data file to wav file .
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <linux/input.h>
#include <pthread.h>
#include <sys/stat.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <unistd.h>

#include "libfmrec.h"

extern int fm_wav_config_header(int fd,
								unsigned int sample_rate,
								unsigned int bits_per_sample,
								unsigned int channels,
								wav_header *header);
extern unsigned int fm_wav_write_data(int fd, char *buffer, unsigned int size);
extern int fm_wav_write_header(int fd, int read_total, int extraChunkSize, wav_header header);

static unsigned char voice_buffer[FM1388_BUFFER_LEN] = { 0 };
static unsigned char voice_buffer_temp[FM1388_BUFFER_LEN] = { 0 };

//functions for transform channel data
int swap_spi_data(unsigned char* pdata, unsigned int data_length, int frame_size, const unsigned char* channels)
{
	unsigned int i, j, offset;
	unsigned int block_number;
	unsigned char  temp;

	if ((pdata == NULL) || (data_length == 0)) {
		printf("%s: please provide valid parameter. pdata=0x%#x, data_length =%#x\n",
			__func__, *pdata, data_length);
		return -EPARAMINVAL;
	}

	block_number = data_length >> 3;

	for (i = 0; i < block_number; i++) {
		offset = i * 8;
		for (j = 0; j < 4; j++) {
			temp = pdata[offset + j];
			pdata[offset + j] = pdata[offset + 7 - j];
			pdata[offset + 7 - j] = temp;
		}
	}

	//add "deadbeef" mark to debug channel
	if(channels[DSP_SPI_REC_CH_NUM - 1] == '1') {
		int pos 	= data_length - 1;
		int mark_pos= data_length - frame_size;
		int index 	= 0;
		
		for(index = 0; index < frame_size; index++) {
			pdata[pos - i + 4] = pdata[pos - i];
		}

		pdata[mark_pos] 	= (unsigned char)0xde;
		pdata[mark_pos + 1] = (unsigned char)0xad;
		pdata[mark_pos + 2] = (unsigned char)0xbe;
		pdata[mark_pos + 3] = (unsigned char)0xef;
	}
	
	return 0;
}

void transform2wavformat(unsigned int ch_num, unsigned int framesize) {
	unsigned int  sr_num, i, j;
	unsigned short* pShortDest;
	unsigned short* pShortSource;

	pShortDest = (unsigned short*)&voice_buffer;
	pShortSource = (unsigned short*)&voice_buffer_temp;
	sr_num = framesize >> 1;//bytes to word

	memset(voice_buffer, 0, sizeof(char) * FM1388_BUFFER_LEN);
	for (i = 0; i < sr_num; i++) {
		for (j = 0; j < ch_num; j++){
			*pShortDest++ = *(pShortSource + i + j * sr_num);
		}
	}
}

//add extra chunk to identify recorded channel information
int init_extra_chunk(char* data_chunk, const unsigned char* channels) {
	int k 			= 0;
	int realIndex 	= 0;
	int len 		= 0;
	
	memset(data_chunk, 0xFE, DSP_SPI_REC_EXTRA_CHUNK_SIZE * sizeof(char));
	strcpy(data_chunk, "{\"channels_label\":[");
	for(k = 0; k < DSP_SPI_REC_CH_NUM; k++) {
		if(channels[k] == '1') {
			if(realIndex == 0) {
				strcat(data_chunk, "\"");
				strcat(data_chunk, (char*)channelName[k]);
				strcat(data_chunk, "\"");
			}
			else {
				strcat(data_chunk, ",\"");
				strcat(data_chunk, (char*)channelName[k]);
				strcat(data_chunk, "\"");
			}
			realIndex++;
		}
	}
	strcat(data_chunk, "]}");

	len 			= strlen(data_chunk);
	data_chunk[len] = 0;
	
	return ESUCCESS;
}

int convert_data_core(const int fd_in, const int fd_out, 
				unsigned int sample_rate, unsigned int bits_per_sample, 
				int channel_number, const unsigned char* channels, int frame_size) {
	int				read_num 		= 0;
	int				total_read_num	= 0;
	int				need_read		= frame_size * channel_number;
	int				ret				= -1;
	wav_header 		header;
	int				read_total 		= 0;
	char extra_chunk[DSP_SPI_REC_EXTRA_CHUNK_SIZE] = { 0 };
	char chunk_info[DSP_SPI_REC_EXTRA_CHUNK_HEADER] = { 0 };

	if((fd_in <= 0) || (fd_out <= 0)) {
		printf("%s:  file handle is invalid. \n", __func__);
		return -ENOTOPEN;
	}

	if((channel_number <= 0) || (channel_number > DSP_SPI_REC_CH_NUM) || 
		(frame_size < 0) || (frame_size > 1000)){
		printf("%s:  parameter is invalid. channel_number=%d, frame_size=%d\n", __func__, channel_number, frame_size);
		return -EPARAMINVAL;
	}

	if(((sample_rate != 8000) && (sample_rate != 16000) && (sample_rate != 24000) && (sample_rate != 48000)) || 
		((bits_per_sample != 8) && (bits_per_sample != 16) && (bits_per_sample != 24) && (bits_per_sample != 32))){
		printf("%s:  sample rate or bit length is invalid. sample_rate=%d, bits_per_sample=%d\n", __func__, sample_rate, bits_per_sample);
		return -EPARAMINVAL;
	}

	ret = fm_wav_config_header(fd_out, sample_rate, bits_per_sample, channel_number, &header);
	
	memset(voice_buffer_temp, 0, sizeof(char) * FM1388_BUFFER_LEN);
	read_num = read(fd_in, voice_buffer_temp, sizeof(char) * need_read);
	while (read_num > 0) {
		total_read_num = read_num;
		while ((read_num > 0) && (total_read_num != need_read)) {
			read_num = read(fd_in, voice_buffer_temp + total_read_num, sizeof(char) * (need_read - total_read_num));
			total_read_num += read_num;
		}

		if (read_num <= 0) { break; }
		if (total_read_num != need_read) { break; }

		ret = swap_spi_data(voice_buffer_temp, total_read_num, frame_size, channels);
		if (ret == 0) {
			transform2wavformat(channel_number, frame_size);
			ret = fm_wav_write_data(fd_out, (char*)voice_buffer, need_read);
		}
		else {
			printf("%s:  swap_spi_data() failed. ret=%d\n", __func__, ret);
			return ret;
		}

		read_total += need_read;

		memset(voice_buffer_temp, 0, sizeof(char) * FM1388_BUFFER_LEN);
		read_num = read(fd_in, voice_buffer_temp, sizeof(char) * need_read);
	}
	
	//add extra chunk
	init_extra_chunk(extra_chunk, channels);
	strcpy(chunk_info, "CHLB");
	chunk_info[4] = (char)((DSP_SPI_REC_EXTRA_CHUNK_SIZE >> 0) & 0xFF);
	chunk_info[5] = (char)((DSP_SPI_REC_EXTRA_CHUNK_SIZE >> 8) & 0xFF);
	chunk_info[6] = (char)((DSP_SPI_REC_EXTRA_CHUNK_SIZE >> 16) & 0xFF);
	chunk_info[7] = (char)((DSP_SPI_REC_EXTRA_CHUNK_SIZE >> 24) & 0xFF);
	
	fm_wav_write_data(fd_out, (char*)chunk_info, DSP_SPI_REC_EXTRA_CHUNK_HEADER);
	fm_wav_write_data(fd_out, (char*)extra_chunk, DSP_SPI_REC_EXTRA_CHUNK_SIZE);
	
	ret = fm_wav_write_header(fd_out, read_total, 
							DSP_SPI_REC_EXTRA_CHUNK_SIZE + DSP_SPI_REC_EXTRA_CHUNK_HEADER, 
							header);

	return ESUCCESS;
}


int convert_data(const char* data_file_name, const char* wav_file_name, 
				unsigned int sample_rate, unsigned int bits_per_sample, 
				int channel_number, const unsigned char* channels, int frame_size) {
	int	fd 		= -1;
	int	fd_out 	= -1;
	int ret 	= -1;

	fd_out = open(wav_file_name, O_CREAT | O_RDWR, 0666);
	if (fd_out == -1) {
		printf("fail to open output file\n");
		return -EFAILOPEN;
	}

	fd = open(data_file_name, O_RDONLY);
	if (fd == -1) {
		printf("fail to open data file\n");
		close(fd_out);
		return -EFAILOPEN;
	}

	ret = convert_data_core(fd, fd_out, sample_rate, bits_per_sample, channel_number, channels, frame_size);
	close(fd);
	close(fd_out);
	return ret;
}


//parse mode cfg file funciton
void parser_mode(char* src_argv, char* delim, cfg_mode_cmd* data)
{
	char  s[CONFIG_LINE_LEN] 	= {0};
	char* pch;
	char  pargc 	= 0;
	int   i 		= 0;

	while ((pch = strsep(&src_argv, delim)) != NULL) {
		if(pargc == 0) {
			data->mode = strtoul(pch, NULL, 16);
		}
		else if (pargc == 1) {
			strncpy(data->path_setting_file_name, pch, MAX_FILENAME_LEN - 1);
		}
		else if (pargc == 2) {
			strncpy(data->mode_setting_file_name, pch, MAX_FILENAME_LEN - 1);
		}
		else if (pargc == 3) {
			strncpy(data->comment, pch, MAX_COMMENT_LEN - 1);
		}
		pargc++;
	}
}

void del_space(char *src)
{
	char* temp = src;

	while (*src) {
		if (*src != ' ') { 
			*temp = *src;
			temp++;
		}
		if(*src == '\0') {
			*temp = '\0' ; 
			return;
		}
		src++;
	}
	*temp = '\0' ; 
}

char* cut_string_from_char(char* str_src, unsigned char special_char) {
	char* temp_ptr = NULL;

	if(str_src == NULL) return NULL;
	
	temp_ptr = strchr(str_src, special_char);
	if(temp_ptr){
		str_src[temp_ptr - str_src] = 0;
	}

	return str_src;
}

char* remove_col_comment(char* str_src, int len)
{
	if(str_src == NULL) return NULL;
	
	str_src = cut_string_from_char(str_src, '#');
	str_src = cut_string_from_char(str_src, '/');
	str_src = cut_string_from_char(str_src, '\r');
	str_src = cut_string_from_char(str_src, '\n');
	str_src[strlen(str_src)] = '\0';

	return str_src;
}

char* replace_tab_as_space(char* str_src, int len)
{
	char* tab;

	tab = strchr(str_src, '\t');
	while(tab) {
		*tab = ' ';
		tab = strchr(str_src, '\t');
	}

	return str_src;
}



char filegetc(int fd)
{
	int cnt;
	char c;

	if(fd == -1) return 0xFF;
	
	cnt = read(fd, &c, 1);
	if (cnt <= 0){
		return 0xFF;
	}
    else{
		return c;
    }
}

int filegets(int fd, char *dst, int max)
{
	char   	c = 0;
	char* 	p;
	int		count = 0;

	if(fd == -1) return 0;

	/* get max bytes or upto a newline */
	count = 0;
	for (p = dst, max--; max > 0; max--) {
		if ((c = filegetc (fd)) == 0xFF) {
			break;
		}
		*p++ = c;
        count++;
		if ((c == '\n') || (c == '\r'))
			break;
	}

    // add end char '\0' in the string
    *p = 0;

    if (p == dst)
		return 0;

	return count;
}


char **remove_row_comment(char* file_name, int* count)
{
	char   s[CONFIG_LINE_LEN] = {0};
	int    fd			= -1;
	char** dst_argv 	= NULL;
	int    line_index 	= 0;
	int    line_number 	= 0;
	int    num = -1;
	
	if(file_name == NULL) {
		printf ("   Got invalid file name.\n");
		return NULL;
	}
	
	fd = open (file_name, O_RDONLY);
	if (fd == -1) {
		printf ("   Could not open the file:%s.\n", file_name);
	} else {
		num = filegets(fd, s, CONFIG_LINE_LEN);
		while (num != 0) {
			if(s[0] == '#' || s[0] == '/' || s[0] == 0x0D || s[0] == 0x0A) {
				//continue;
			} else {
				(*count)++;
			}
			num = filegets(fd, s, CONFIG_LINE_LEN);
		}
		
		line_number = (*count);
		dst_argv = (char**)malloc((line_number + 1) * sizeof(char*));
		memset(dst_argv, 0, line_number * sizeof(char*));

		if(dst_argv == NULL) {
			printf("   Failed to alloc memory.");
			close (fd);
			return NULL;
		}
		
		lseek(fd, 0, SEEK_SET);
		
		line_index = 0;
		while (filegets(fd, s, CONFIG_LINE_LEN) != 0) {
			if((s[0] == '#') || (s[0] == '/') || (s[0] == 0x0d) || (s[0] == 0x0a)) {
				continue;
			} else {
				dst_argv[line_index] = (char*)malloc((unsigned int)(strlen(s) + 1));
				memset(dst_argv[line_index], 0, (strlen(s) + 1) * sizeof(char));
				strncpy(dst_argv[line_index], s, (unsigned int)strlen(s));
				line_index++;
			}
		}

		close (fd);

		return dst_argv;
	}
	
	return NULL;
}

int load_fm1388_mode_cfg(char* file_src, support_mode* mode_info)
{
	int 			ret 			= ESUCCESS;
	unsigned char  	argc 			= 0;
	int 			i, cmd_count 	= 0;
	char*  			delim 			= ",";
	char** 			processed_argv	= NULL;
	cfg_mode_cmd 	cfg_mode;

	processed_argv = remove_row_comment(file_src, &cmd_count);
	if(cmd_count > MAX_MODE_NUM) {
		cmd_count = MAX_MODE_NUM;
	}
	
	if(processed_argv == NULL) {
		printf("   Can not get configuration data from file: %s\n", file_src);
		ret = -EFAILOPEN;
	} else {
		mode_info->number = cmd_count;
		for(i = 0; i < cmd_count; i++) {
			//need to convert \t(0x09) to space(0x20)
			processed_argv[i] = replace_tab_as_space(processed_argv[i], strlen(processed_argv[i]));
			if(processed_argv[i]) {
				processed_argv[i] = remove_col_comment(processed_argv[i], strlen(processed_argv[i]));
				if(processed_argv[i]) {
					del_space(processed_argv[i]);
					if(processed_argv[i]) {
						parser_mode(processed_argv[i], delim, &cfg_mode);
						mode_info->mode[i] = cfg_mode.mode;
						strncpy(mode_info->comment[i], cfg_mode.comment, MAX_COMMENT_LEN - 1);
					}
				}
			}
		}
	}

	for(i = 0; i < cmd_count; i++) {
		if(processed_argv[i] != NULL) free(processed_argv[i]);
		processed_argv[i] = NULL;
	}
	
	if(processed_argv != NULL) free(processed_argv);
	processed_argv = NULL;
	
	return ret;
}

//translate DSP sample rate data to real sample rate
int get_dsp_sample_rate(int sample_rate) {
	int cur_sample_rate = 0;
	int sample_rate_index = sample_rate & 0xFFFF;
	
	if((sample_rate_index < 0) || (sample_rate_index > 0x3)) { 
		printf("sample_rate value is not valid:%d.", sample_rate_index);
		return -EDATAINVAL;
	}
   
	cur_sample_rate = sample_rate_list[sample_rate_index];
	return cur_sample_rate;
}	
