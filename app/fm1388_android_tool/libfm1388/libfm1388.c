/*
 * external/libfm1388/libfm1388.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * let fm_fm1388 application communicate with FM1388 driver.
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
#include <string.h>

#include "libfm1388.h"

#define DEVICE_NAME 		 "/dev/fm1388"
#define CHAR_DEVICE_NAME 	 "/dev/fm1388_smp1"


static unsigned int frame_size = 0;
static unsigned int channel_num = 0;
static unsigned int rec_channel_num = 0;
static int dev_fd = -1;
static int dev_fd_cnt = 0;

void strip_white_space(char* source_string, int offset, char* target_string, int len) {
	int i = 0, j = 0;
	
	if((source_string == NULL) || (target_string == NULL) || (offset < 0) || (len <= 0)) return;
	
	i = offset;
	while((source_string[i] != 0) && (i < CONFIG_LINE_LEN) && (j < len)){
		if((source_string[i] != ' ') && (source_string[i] != '\t') && (source_string[i] != '\r') && (source_string[i] != '\n')) {
			target_string[j++] = source_string[i];
		}
		
		i++;
	}
	
	target_string[j] = 0;
	return;
}

// load user-defined path setting file, if file does not exist use default setting
int load_user_setting(void)
{
    int		fd = -1;
    int 	num = 0;
    char 	firmware_path[CONFIG_LINE_LEN];	
    char 	user_file_path[CONFIG_LINE_LEN];	
    char 	s[CONFIG_LINE_LEN];	//assume each line of the opened file is below 255 characters

	memset(firmware_path, 0, CONFIG_LINE_LEN);
	memset(user_file_path, 0, CONFIG_LINE_LEN);
	
	//set default path to SDcard and mode path, then parse config file to get user-defined path to replace these path
	strncpy(SDCARD_PATH, DEFAULT_SDCARD_PATH, MAX_PATH_LEN);
	strncpy(firmware_path, DEFAULT_MODE_CFG_LOCATION, CONFIG_LINE_LEN);
	strncpy(MODE_CFG_LOCATION, DEFAULT_MODE_CFG_LOCATION, CONFIG_LINE_LEN);
	strncat(MODE_CFG_LOCATION, MODE_CFG_FILE_NAME, CONFIG_LINE_LEN);
	
	strncpy(user_file_path, firmware_path, CONFIG_LINE_LEN);
	strncat(user_file_path, USER_DEFINED_PATH_FILE, CONFIG_LINE_LEN);
	
	//printf("%s: load user-defined path file %s\n", __func__, user_file_path);
	
	fd = open (user_file_path, O_RDONLY);
    if (fd == -1) {
       printf ("%s, File %s could not be opened or does not exist, will use default setting for sdcard and vec location.\n", __func__, user_file_path);
       return -EFAILOPEN;
    }
    else{
		//printf ("%s, File %s opened!...\n", __func__, user_file_path);
		num = filegets(fd, s, CONFIG_LINE_LEN);
		while (num != 0) {
			if(s[0] == '*' || s[0] == '#' || s[0] == '/' || s[0] == 0xD || s[0] == 0x0) {
				//continue;
			} else {
				if(strncasecmp(s, USER_VEC_PATH, strlen(USER_VEC_PATH)) == 0) {
					strip_white_space(s, strlen(USER_VEC_PATH), MODE_CFG_LOCATION, CONFIG_LINE_LEN);
					strncat(MODE_CFG_LOCATION, MODE_CFG_FILE_NAME, CONFIG_LINE_LEN);
					printf("%s: mode file path is %s\n", __func__, MODE_CFG_LOCATION);
				}
				else if(strncasecmp(s, USER_KERNEL_SDCARD_PATH, strlen(USER_KERNEL_SDCARD_PATH)) == 0) {
					//not need parse kernel space sd card path
				}
				else if(strncasecmp(s, USER_USER_SDCARD_PATH, strlen(USER_USER_SDCARD_PATH)) == 0) {
					strip_white_space(s, strlen(USER_USER_SDCARD_PATH), SDCARD_PATH, MAX_PATH_LEN);
					printf("%s: sdcard path is %s\n", __func__, SDCARD_PATH);
				}
			}
			num = filegets(fd, s, CONFIG_LINE_LEN);
		}
    }

	close(fd);
   
	return ESUCCESS;
}

char* get_cfg_location(void) {
	if((MODE_CFG_LOCATION == NULL) || (MODE_CFG_LOCATION[0] == 0)) 
		load_user_setting();
	
	return MODE_CFG_LOCATION;
}

char* get_sdcard_path(void) {
	if((SDCARD_PATH == NULL) || (SDCARD_PATH[0] == 0)) 
		load_user_setting();
	
	return SDCARD_PATH;
}

//open device and keep its handle for further calling
//application which call lib_open_device should call lib_close_device() to close the handle
int lib_open_device(void)
{
	load_user_setting();
	
	if(dev_fd != -1) {
		dev_fd_cnt++;
		return dev_fd;
	}
	
	dev_fd = open(DEVICE_NAME, O_RDWR);  
	if(dev_fd == -1) {  
		printf("Failed to open device %s.\n", DEVICE_NAME);
		return -EFAILOPEN;
	}
	dev_fd_cnt++;

	return ESUCCESS;
}

//close the device handle
bool lib_close_device(void)
{
	if(dev_fd != -1) {
		dev_fd_cnt--;
		if(dev_fd_cnt == 0) {
			close(dev_fd);
			dev_fd = -1;
		}
	}
	else{
		return false;
	}
	
	return true;
}

//get dsp mode
int get_mode(void)
{
	dev_cmd_mode_gs local_dev_cmd;
	
	if(dev_fd == -1) return -ENOTOPEN;
	
	local_dev_cmd.cmd_name = FM_SMVD_MODE_GET;
	local_dev_cmd.dsp_mode = 0;
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_mode_gs));

	return local_dev_cmd.dsp_mode;
}

//set dsp mode
int set_mode(int dsp_mode)
{
	dev_cmd_mode_gs local_dev_cmd;

	if(dev_fd == -1) return -ENOTOPEN;
	
	local_dev_cmd.cmd_name = FM_SMVD_MODE_SET;
	local_dev_cmd.dsp_mode = dsp_mode;
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return ESUCCESS;
}

//get register value
int get_reg_value(int reg_addr)
{
	dev_cmd_reg_rw local_dev_cmd;

	if(dev_fd == -1) return -ENOTOPEN;
	
	local_dev_cmd.cmd_name = FM_SMVD_REG_READ;
	local_dev_cmd.reg_addr = reg_addr;
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return (local_dev_cmd.reg_val & 0xFFFF);
}

//set value to register 
int set_reg_value(int reg_addr, int value)
{
	dev_cmd_reg_rw local_dev_cmd;

	if(dev_fd == -1) return -ENOTOPEN;
	
	local_dev_cmd.cmd_name = FM_SMVD_REG_WRITE;
	local_dev_cmd.reg_addr = reg_addr;
	local_dev_cmd.reg_val  = value;
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return ESUCCESS;
}

//get value from dsp memory
int get_dsp_mem_value(unsigned int mem_addr)
{
	dev_cmd_short local_dev_cmd;
	
	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name = FM_SMVD_DSP_ADDR_READ;
	local_dev_cmd.addr = mem_addr;
	
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return (local_dev_cmd.val & 0xFFFF);
}

//set value to dsp memory
int set_dsp_mem_value(unsigned int mem_addr, int value)
{
	dev_cmd_short local_dev_cmd;
	
	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_WRITE;
	local_dev_cmd.addr 		= mem_addr;
	local_dev_cmd.val 		= value;
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short)); 

	return ESUCCESS;    
}

//get value of memory address via spi
int get_dsp_mem_value_spi(unsigned int mem_addr)
{
	dev_cmd_short local_dev_cmd;
	
	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_READ_SPI;
	local_dev_cmd.addr 		= mem_addr;
	
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return local_dev_cmd.val;
}

//set value to memory address via spi
int set_dsp_mem_value_spi(unsigned int mem_addr, int value) {
	dev_cmd_short local_dev_cmd;

	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_WRITE_SPI;
	local_dev_cmd.addr 		= mem_addr;
	local_dev_cmd.val 		= value;
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return ESUCCESS;    
}

int convert_channel_index(unsigned int ch_num, unsigned char* ch_idx, unsigned short* final_ch_num, unsigned char* final_ch_idx) {
	if(final_ch_idx == NULL) return -EPARAMINVAL;
	if(final_ch_num == NULL) return -EPARAMINVAL;
	
	if((ch_num > 0) && (ch_num <= DSP_SPI_REC_CH_NUM)) {
		*final_ch_num	= ch_num;
		
		if(ch_idx) {
			memcpy(final_ch_idx, ch_idx, sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
		else {
			*final_ch_num	= DSP_SPI_REC_CH_NUM;
			memset(final_ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
	}
	else {
		*final_ch_num	= DSP_SPI_REC_CH_NUM;
		memset(final_ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
	}

	return ESUCCESS;
}
//start to record
//ch_idx is channel array, '1' means record the related channel, ' ' means omit this channel
int start_debug_record(unsigned int ch_num, unsigned char* ch_idx, char* filepath)
{
	int ret_val = ESUCCESS;
	dev_cmd_start_rec local_dev_cmd;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	int i;
	
	if(dev_fd == -1) return -ENOTOPEN;
	if(filepath == NULL) return -EPARAMINVAL;
	
	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
	unlink(data_file_path);

	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_FETCH_VDATA_START;
	
	ret_val = convert_channel_index(ch_num, ch_idx, &(local_dev_cmd.ch_num), local_dev_cmd.ch_idx);
	if(ret_val != ESUCCESS) {
		return ret_val;
	}
/*	
	if((ch_num > 0) && (ch_num <= DSP_SPI_REC_CH_NUM)) {
		local_dev_cmd.ch_num	= ch_num;
		
		if(ch_idx) {
			memcpy(local_dev_cmd.ch_idx, ch_idx, sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
		else {
			local_dev_cmd.ch_num	= DSP_SPI_REC_CH_NUM;
			memset(local_dev_cmd.ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
	}
	else {
		local_dev_cmd.ch_num	= DSP_SPI_REC_CH_NUM;
		memset(local_dev_cmd.ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
	}
*/	

	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		return -EDATAINVAL;
	}
	
	channel_num = ch_num;
	
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_start_rec));
	if(ret_val < 0) {
		printf("error occurs when start recording. error code=%d\n", ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

//stop recording and convert voice data into wav format then save data to file. 
/*Fuli 20160926 we will get sample rate from DSP instead of input by user. will hardcode bits_per_sample to 16
int stop_debug_record(char* wav_file_name, unsigned int sample_rate, 
						unsigned int bits_per_sample) {
*/
int stop_debug_record(const char* wav_file_name, const unsigned char* channels) {
	dev_cmd_short local_dev_cmd;
	int ret_val 		= ESUCCESS;
	int cur_sample_rate = -1;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	char wav_file_full_path[MAX_PATH_LEN] = { 0 };
	
	if(dev_fd == -1) return -ENOTOPEN;
	if(wav_file_name == NULL) return -EPARAMINVAL;
	if(channels == NULL) return -EPARAMINVAL;
	
	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_FETCH_VDATA_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val <= 0) {
		printf("Error occurs when stop recording data.\n");
		return ret_val;
	}
	
	//get sample rate from DSP and hardcode bits_per_sample as 16
	cur_sample_rate = get_dsp_mem_value(DSP_SAMPLE_RATE_ADDR);
	if(cur_sample_rate < 0) {
		printf("got invalid sample rate data:%x\n", cur_sample_rate);
		return -EDATAINVAL;
	}
	
	cur_sample_rate = get_dsp_sample_rate(cur_sample_rate);
	if(cur_sample_rate < 0) { 
		printf("sample_rate value is not valid:%d.", cur_sample_rate);
		return -EDATAINVAL;
	}

  
	snprintf(wav_file_full_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, wav_file_name);
	unlink(wav_file_full_path);
	
 	usleep(5000); //wait for driver stop the recording
	ret_val = convert_data(data_file_path, wav_file_full_path, cur_sample_rate, 16, 
							channel_num, channels, frame_size);
	if(ret_val) {
		printf("Error occurs when save recording data. error code=%d\n", ret_val);
		return ret_val;
	}

	return ESUCCESS;
}

//just for ADB Tool
int stop_debug_record_by_ADBTool(const char* wav_file_name, const unsigned char* channels, int channel_number) {	
	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		return -EDATAINVAL;
	}
	
	channel_num = channel_number;
	
	return stop_debug_record(wav_file_name, channels);
}

int stop_debug_record_force(void) {
	dev_cmd_short local_dev_cmd;
	int ret_val 		= ESUCCESS;
	
	if(dev_fd == -1) return -ENOTOPEN;
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_FETCH_VDATA_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val <= 0) {
		printf("Error occurs when stop recording data.\n");
		return ret_val;
	}
	
	return ESUCCESS;
}

/************************ function and interface for SPI Playback *****************************/
int get_channel_number(char* file_path) {
	FILE* 		wav_fp = NULL;
	char 		play_file_full_path[MAX_PATH_LEN + 1] 	= {0};
	wav_header	header;
	int 		ret = 0;
	
	if(file_path == NULL) {
		printf(" - Invalid file path.\n");
		return -EPARAMINVAL;
	}
	
	snprintf(play_file_full_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, file_path);
	wav_fp = fopen(play_file_full_path, "rb");
	if (wav_fp == NULL) {
		printf(" - Fail to open the input wav file, please make sure it exist and can be read.\n");
		return -EFAILOPEN;
	}

	ret = fread(&header, sizeof(wav_header), 1, wav_fp);

	if (ret <= 0) {
		printf(" - Error occurs when read wav file header.\n");
		return -EFILECANNOTREAD;
	}
	
	return header.num_channels;
}

bool check_channel_mapping(int channel_num, char* channel_mapping) {
	int i, j = 0;
	int len = 0;
	
	if(channel_mapping == NULL) return false;
	if(channel_num == 0) return false;
	
	len = strlen(channel_mapping);
	if((len % 3) != 0) return false;
	
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			if((channel_mapping[j] - '0' + 1) > channel_num)  break;
			j ++;
			
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(strncasecmp(channel_mapping + j, valid_channels[i], 2) == 0)	break;
			}
			if(i == MAX_MAP_CH_NUM) break;
		}
		else break;
		
		j += 2;
	}
	
	if(j < len) return false;
	
	return true;
}

int trans_mapping_str(char* channel_mapping, char* trans_mapping_str) {
	int i, j, len;

	if(channel_mapping == NULL) return -EPARAMINVAL;
	if(trans_mapping_str == NULL) return -EPARAMINVAL;
	
	//translate channel mapping string
	len = strlen(channel_mapping);
	j = 0;
	memset(trans_mapping_str, '-', DSP_SPI_REC_CH_NUM);
	trans_mapping_str[DSP_SPI_REC_CH_NUM] = 0;
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(strncasecmp(channel_mapping + j + 1, valid_channels[i], 2) == 0)	break;
			}
			if(i == MAX_MAP_CH_NUM) break;
			
			trans_mapping_str[channel_mapping[j] - '0'] = i + '0';
		}
		else break;
		
		j += 3;
	}
	
	if(j < len) {
		printf("channel mapping string format is wrong.\n");
		return -EPARAMINVAL;
	}
	
	return ESUCCESS;
}
//start playback
//channel_mapping likes 1M02M23AL4AR
int start_spi_playback(char* channel_mapping, char* filepath)
{
	int ret_val = ESUCCESS;
	dev_cmd_spi_play local_dev_cmd;
	char trans_ch_mapping[DSP_SPI_REC_CH_NUM + 1];
	
	if(dev_fd == -1) return -ENOTOPEN;
	if(filepath == NULL) return -EPARAMINVAL;
	if(channel_mapping == NULL) return -EPARAMINVAL;

/* Originally, I want to check the channel number and 
 * channel mapping string validation in here for both app and apk
 * run app in linux console	or adb console, it is ok. 
 * but when library is calling by HAL from APK, the file can not be opened
 * because it is in android space, the path /sdcard/ is not valid, so the file
 * under path /sdcard/ can not be opened.
 * So the channel number and channel mapping string checking work should be done in app and apk seperately.
 * Otherwise, APK and APP should pass its SDCARD path as parameter to this function. such as:
 * in APP, it is /sdcard/ or /mnt/sdcard/
 * in APK, it is /storage/emulated/0/ or /mnt/sdcard/
 */
/*
	ch_num = 0;
	ch_num = get_channel_number(filepath);
	if(ch_num <= 0) {
		printf("   Fail to parse the wav file you provided.\n");
		return ch_num;
	}

	if(!check_channel_mapping(ch_num, channel_mapping)) { //contains invalid character
		printf("   Channel Mapping is invalid.\n");
		return -EPARAMINVAL;
	} 
*/
	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_PLAYBACK_START;
	strncpy(local_dev_cmd.file_path, filepath, MAX_PATH_LEN);
	
	//translate channel mapping string
	ret_val = trans_mapping_str(channel_mapping, trans_ch_mapping);
	if(ret_val != ESUCCESS) {
		return ret_val;
	}
	
/*
	len = strlen(channel_mapping);
	j = 0;
	memset(trans_ch_mapping, '-', DSP_SPI_REC_CH_NUM);
	trans_ch_mapping[DSP_SPI_REC_CH_NUM] = 0;
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(strncasecmp(channel_mapping + j + 1, valid_channels[i], 2) == 0)	break;
			}
			if(i == MAX_MAP_CH_NUM) break;
			
			trans_ch_mapping[channel_mapping[j] - '0'] = i + '0';
		}
		else break;
		
		j += 3;
	}
	
	if(j < len) {
		printf("channel mapping string format is wrong.\n");
		return -EPARAMINVAL;
	}
*/

	
printf("channel mapping string is translated to: %s\n", trans_ch_mapping);
	strncpy(local_dev_cmd.channel_mapping, trans_ch_mapping, DSP_SPI_REC_CH_NUM);
	
	local_dev_cmd.need_recording = 0;

	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_spi_play));
	if(ret_val < 0) {
		printf("error occurs when start playback. error code=%d\n", ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

int start_spi_playback_by_ADBTool(char* channel_mapping, char* filepath, unsigned char cPlayMode, unsigned char cPlayOutput) {
	return start_spi_playback(channel_mapping, filepath);
}

int stop_spi_playback() {
	dev_cmd_short local_dev_cmd;
	int ret_val = ESUCCESS;
	
	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name = FM_SMVD_DSP_PLAYBACK_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when stop playback.error code=%d\n", ret_val);
		return ret_val;
	}

	return ESUCCESS;
}

int start_spi_playback_rec(char* channel_mapping, char* filepath, char need_recording, unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath) {
	int ret_val = ESUCCESS;
	dev_cmd_spi_play local_dev_cmd;
	char trans_ch_mapping[DSP_SPI_REC_CH_NUM + 1];
	char data_file_path[MAX_PATH_LEN] = { 0 };
	
	if(dev_fd == -1) return -ENOTOPEN;
	if(filepath == NULL) return -EPARAMINVAL;
	if(channel_mapping == NULL) return -EPARAMINVAL;
	
	if((need_recording == 1) && (rec_filepath == NULL)) return -EPARAMINVAL;

	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_PLAYBACK_START;
	strncpy(local_dev_cmd.file_path, filepath, MAX_PATH_LEN);
	
	//translate channel mapping string
	ret_val = trans_mapping_str(channel_mapping, trans_ch_mapping);
	if(ret_val != ESUCCESS) {
		return ret_val;
	}
	
	//printf("channel mapping string is translated to: %s\n", trans_ch_mapping);
	strncpy(local_dev_cmd.channel_mapping, trans_ch_mapping, DSP_SPI_REC_CH_NUM);

	local_dev_cmd.need_recording = need_recording;

	if(need_recording == 1) {	
		snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
		unlink(data_file_path);

		ret_val = convert_channel_index(rec_ch_num, rec_ch_idx, &(local_dev_cmd.rec_ch_num), local_dev_cmd.rec_ch_idx);
		if(ret_val != ESUCCESS) {
			return ret_val;
		}
		//printf("convert_channel_index string is: %s\n", local_dev_cmd.rec_ch_idx);
		
		frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
		if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
			printf("framesize is not correct. frame_size=%d\n", frame_size);
			return -EDATAINVAL;
		}

		rec_channel_num = rec_ch_num;
	}	

	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_spi_play));
	if(ret_val < 0) {
		printf("error occurs when start playback. error code=%d\n", ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

int start_spi_playback_rec_by_ADBTool(char* channel_mapping, char* filepath, char need_recording, 
					unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath,
					unsigned char cPlayMode, unsigned char cPlayOutput) {
	return start_spi_playback_rec(channel_mapping, filepath, need_recording, rec_ch_num, rec_ch_idx, rec_filepath);
}

int stop_spi_playback_rec(const char* wav_file_name, const unsigned char* channels) {
	dev_cmd_short local_dev_cmd;
	int ret_val = ESUCCESS;
	int cur_sample_rate = -1;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	char wav_file_full_path[MAX_PATH_LEN] = { 0 };
	int rec_ch_num = 0, i;
	
	if(dev_fd == -1) return -ENOTOPEN;
	if(wav_file_name == NULL) return -EPARAMINVAL;
	if(channels == NULL) return -EPARAMINVAL;

	local_dev_cmd.cmd_name = FM_SMVD_DSP_PLAYBACK_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when stop playback.error code=%d\n", ret_val);
		return ret_val;
	}

	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, VOICE_DATA_FILE_NAME);
	
	//get sample rate from DSP and hardcode bits_per_sample as 16
	cur_sample_rate = get_dsp_mem_value(DSP_SAMPLE_RATE_ADDR);
	if(cur_sample_rate < 0) {
		printf("got invalid sample rate data:%x\n", cur_sample_rate);
		return -EDATAINVAL;
	}
	
	cur_sample_rate = get_dsp_sample_rate(cur_sample_rate);
	if(cur_sample_rate < 0) { 
		printf("sample_rate value is not valid:%d.", cur_sample_rate);
		return -EDATAINVAL;
	}

	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		return -EDATAINVAL;
	}

	rec_ch_num = 0;
	for(i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
		if(channels[i] == '1') rec_ch_num ++;
	}

	snprintf(wav_file_full_path, MAX_PATH_LEN, "%s%s", SDCARD_PATH, wav_file_name);
	unlink(wav_file_full_path);
   
	usleep(5000); //wait for driver stop the recording
printf("voice data file path:%s, wav file path:%s\n", data_file_path, wav_file_full_path);
	ret_val = convert_data(data_file_path, wav_file_full_path, cur_sample_rate, 16, 
							rec_ch_num, channels, frame_size);
	if(ret_val) {
		printf("Error occurs when save recording data. error code=%d\n", ret_val);
		return ret_val;
	}

	return ESUCCESS;
}

int is_playing() {
	dev_cmd_short local_dev_cmd;
	int ret_val = ESUCCESS;
	
	if(dev_fd == -1) return -ENOTOPEN;

	local_dev_cmd.cmd_name = FM_SMVD_DSP_IS_PLAYING;
	ret_val = read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when check playing status. error code=%d\n", ret_val);
		return ret_val;
	}

	return local_dev_cmd.val;
}


