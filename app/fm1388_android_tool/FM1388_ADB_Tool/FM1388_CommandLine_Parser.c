#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "FM1388_CommandLine_Parser.h"
#include "../libfm1388/libfm1388.h"
#include "../FM1388_Parameter_Lib/libFM1388Parameter.h"

extern int process_read_write_data(char* request_parameter_filepath, char* result_filepath);
int execute_command(int command_id, int argc, char** argv);

char	result_str[3][COMMENT_LENGTH]	= { "", "", "" };

int seek_command_id(char* argv) {
	int i = 0;
	
	if(argv == NULL) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL;
	}
	
	for(i = 0; i < MAX_COMMAND_NUM; i++) {
		if(strcasecmp(argv, supported_command[i].strCommand) == 0) break;
	} 
	
	return i;
}

int seek_play_record_parameter(char* argv, char* param) {
	int i = 0;
	int param_len = 0;
	
	
	if((argv == NULL) || (param == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL;
	}
	
	for(i = 0; i < MAX_PLAY_RECORD_PARAM_NUM; i++) {
		param_len = strlen(play_record_parameter[i]);
		if(strncasecmp(argv, play_record_parameter[i], param_len) == 0) {
			if(argv[param_len] == '=') {
				strncpy(param, argv + param_len + 1, MAX_PARAMETER_LEN - 1);
				break;
			}
		}
	} 

	return i;
}

int get_mode_parameter(int argc, char** argv, int* mode) {
	int 	ret = ESUCCESS;
	int 	i = 0;
	char 	strParameter[MAX_PARAMETER_LEN];
	
	if((argc != 3) || (argv == NULL) || (mode == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL; 
	}
	
	for(i = 1; i < argc; i++) {
		if(argv[i] == NULL) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d.\n", __func__, i);
			return -EPARAMINVAL;
		}
		
		if(strlen(argv[i]) <= 2) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d, parameter=%s\n", __func__, i, argv[i]);
			return -EPARAMINVAL;
		}
		
		if((argv[i][0] == '-') && (argv[i][1] == '-')) { //"--yyy" is parameter
			//seek mode
			if(strncasecmp(argv[i] + 2, "mode", 4) == 0) {
				if(argv[i][6] == '=') {
					*mode = strtol(argv[i] + 7, NULL, 16);
					break;
				}
			}
		}
	}
	
	if(i == argc) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Did not find mode from command line parameters\n", __func__);
		return -EPARAMINVAL;
	}
	
	return ret;
}

int get_play_record_parameters(int argc, char** argv, SPIPlayRecord *p_play_record) {
	int 	ret = ESUCCESS;
	int 	i = 0, param_id = -1, j = 0;
	char 	strParameter[MAX_PARAMETER_LEN];
	
	if((argc < 5) || (argv == NULL) || (p_play_record == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL; 
	}
	
	for(i = 1; i < argc; i++) {
		if(argv[i] == NULL) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d.\n", __func__, i);
			return -EPARAMINVAL;
		}
		
		if(strlen(argv[i]) <= 2) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d, parameter=%s\n", __func__, i, argv[i]);
			return -EPARAMINVAL;
		}
		
		if((argv[i][0] == '-') && (argv[i][1] == '-')) { //"--yyy" is parameter
			//seek parameter
			memset(strParameter, 0, MAX_PARAMETER_LEN);
			param_id = seek_play_record_parameter(argv[i] + 2, strParameter);
			if(param_id == MAX_PLAY_RECORD_PARAM_NUM) { //not found
				continue;
			}
			else if((param_id < 0) || (param_id > MAX_PLAY_RECORD_PARAM_NUM)){
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Error occurs when get command type from command line. [%d] = %s\n", __func__, i, argv[i]);
				return -EPARAMINVAL;
			}
			else {
				switch(param_id) {
					case FM1388_ADB_PLAYREC_CMD:
						p_play_record->cOperation 	= strParameter[0];
						p_play_record->cCommand 	= strParameter[1];
						break;

					case FM1388_ADB_PLAYREC_REC_CHANNEL:
						memcpy(p_play_record->strChannelIndex, strParameter, DSP_SPI_CH_NUM * sizeof(char));
						for(j = strlen(strParameter); j < DSP_SPI_CH_NUM; j++) {
							p_play_record->strChannelIndex[j] = '0';
						}
						break;
						
					case FM1388_ADB_PLAYREC_REC_FILE:
						strncpy(p_play_record->strOutputFilePath, strParameter, MAX_FILEPATH_LEN);
						for(j = strlen(strParameter); j < MAX_FILEPATH_LEN; j++) {
							p_play_record->strOutputFilePath[j] = PLACEHOLDER;
						}
						p_play_record->strOutputFilePath[MAX_FILEPATH_LEN] = 0;
						break;
						
					case FM1388_ADB_PLAYREC_PLAY_CHANNEL:
						p_play_record->cChannelNum = 6;
						strncpy(p_play_record->strChannelMapping, strParameter, MAX_MAP_CH_NUM * 3);
						for(j = strlen(strParameter); j < MAX_MAP_CH_NUM * 3; j++) {
							p_play_record->strChannelMapping[j] = PLACEHOLDER;
						}
						p_play_record->strChannelMapping[MAX_MAP_CH_NUM * 3] = 0;
						break;

					case FM1388_ADB_PLAYREC_PLAY_FILE:
						strncpy(p_play_record->strInputFilePath, strParameter, MAX_FILEPATH_LEN);
						for(j = strlen(strParameter); j < MAX_FILEPATH_LEN; j++) {
							p_play_record->strInputFilePath[j] = PLACEHOLDER;
						}
						p_play_record->strInputFilePath[MAX_FILEPATH_LEN] = 0;
						break;

					case FM1388_ADB_PLAYREC_OPTION:
						p_play_record->cPlayMode 	= strParameter[0];
						p_play_record->cPlayOutput 	= strParameter[1];
						break;

					default:
						output_debug_log(true, "[FM1388_CommandLine_Parser--%s], unsupported parameter.\n", __func__);
						break;

				}
			}
		}
	}
	
	return ret;
}

int generate_adb_result(char* result_file_path, int result_num, unsigned short* result_data) {
	int 			ret = ESUCCESS;
	RequestPara		*result_info;
	int 			i = 0;
	
	if((result_file_path == NULL) || (result_num == 0) || (result_data == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL; 
	}
	
	result_info = (RequestPara*)malloc(sizeof(RequestPara) * result_num);
	if (result_info == NULL) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Can not allocate memory for result data.\n", __func__);
		return -ENOMEMORY;
	}
	
	for(i = 0; i < result_num; i++) {
		result_info[i].op 		= 1;
		result_info[i].addr 	= 0xA5A5A5A5;
		result_info[i].value 	= result_data[i];
		strncpy(result_info[i].comment, result_str[i], COMMENT_LENGTH - 1);
	}
		
	generate_result_file(result_file_path, result_info, result_num);

	if (result_info != NULL) free(result_info);
	
	return ret;
}

int generate_fail_adb_result(char* result_filepath) {
	unsigned short	result_val[1] = { 0 };

	result_val[0] = 0;
	memset(result_str[0], 0, COMMENT_LENGTH);
	strncpy(result_str[0], FAILED, COMMENT_LENGTH - 1);
	return generate_adb_result(result_filepath, 1, result_val);
}

//return:
//	0~MAX_COMMAND_NUM-1 for valid command
//	< 0 error
// 	MAX_COMMAND_NUM no supported command
int parse_execute_command(int argc, char** argv) {
	int i = 0, new_format = 0, ret = -1;;
	int command_id = -1;
	
	if(argc == 1) return MAX_COMMAND_NUM;
	
	if((argc < 2) || (argv == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL;
	}
	
	for(i = 1; i < argc; i++) {
		if(argv[i] == NULL) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d.\n", __func__, i);
			return -EPARAMINVAL;
		}
		
		if(strlen(argv[i]) <= 2) {
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter in param %d, parameter=%s\n", __func__, i, argv[i]);
			return -EPARAMINVAL;
		}
		
		if(argv[i][0] == '-') new_format = 1;
		
		if((argv[i][0] == '-') && (argv[i][1] != '-')) { //only "-xxx" is command, "--yyy" is parameter
			//seek command ID
			command_id = seek_command_id(argv[i] + 1);
			if(command_id == MAX_COMMAND_NUM) { //not found
				continue;
			}
			else if((command_id < 0) || (command_id > MAX_COMMAND_NUM)){
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Error occurs when get command type from command line. [%d] = %s\n", __func__, i, argv[i]);
				return -EPARAMINVAL;
			}
			else {
				ret = execute_command(command_id, argc, argv);
				if(ret < 0) {
					return ret;
				}
				else {
					return command_id;
				}
				
				break;
			}
		}
	}
	
	if((new_format == 0) && (i == argc)) return MAX_COMMAND_NUM;
	
	return -EPARAMINVAL;
}

int execute_command(int command_id, int argc, char** argv) {
	int 	ret = ESUCCESS;
	int		i, mode;
	SPIPlayRecord st_play_record;
	char	sdcard_path[MAX_PATH_LENGTH] 				= { 0 };
	char	firmware_path[MAX_PATH_LENGTH] 				= { 0 };
	char 	request_parameter_filepath[MAX_PATH_LENGTH]	= { 0 };
	char 	result_filepath[MAX_PATH_LENGTH]			= { 0 };
	unsigned short	result_val[3]						= { 0 };
	char 	strParamemters[MAX_CMDLINE_LEN]				= { 0 };
	
	if((argc < 2) || (argv == NULL)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Got wrong parameter.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if((command_id < 0) || (command_id >= MAX_COMMAND_NUM)) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s] Invalid command id:%d.\n", __func__, command_id);
		return -EPARAMINVAL;
	}

	strncpy(sdcard_path, get_sdcard_path(), MAX_PATH_LENGTH - 1);
	strncpy(firmware_path, get_cfg_location(), MAX_PATH_LENGTH - 1);
		
	//omit the file name in path
	for(i = strlen(firmware_path) - 1; i > 0; i--) {
		if(firmware_path[i] == '/') break;
	}
	
	if((i <= 0) || (firmware_path[i] != '/')) {
		output_debug_log(true, "[FM1388_CommandLine_Parser--%s], FM1388 firmware and parameter file path is not correct.\n", __func__);
		return -1;
	}
	else {
		firmware_path[i + 1] = 0;
	}	

	//prepare request and result file path
	snprintf(request_parameter_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, request_parameter_filename);
	snprintf(result_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, result_filename);
	
	switch(command_id) {
		case FM1388_ADB_GET_VERSION:
			result_val[0] = 1;
			memset(result_str[0], 0, COMMENT_LENGTH);
			strncpy(result_str[0], ADB_TOOL_VERSION, COMMENT_LENGTH - 1);
			ret = generate_adb_result(result_filepath, 1, result_val);
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], ADB_Tool version is %s.\n", __func__, ADB_TOOL_VERSION);
			break;
			
		case FM1388_ADB_GET_PATH:
			result_val[0] = 1;
			memset(result_str[0], 0, COMMENT_LENGTH);
			strncpy(result_str[0], sdcard_path, COMMENT_LENGTH - 1);
			result_val[1] = 1;
			memset(result_str[1], 0, COMMENT_LENGTH);
			strncpy(result_str[1], firmware_path, COMMENT_LENGTH - 1);
			ret = generate_adb_result(result_filepath, 2, result_val);
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], sdcard path is: %s, vec path is: %s\n", __func__, sdcard_path, firmware_path);
			break;
			
		case FM1388_ADB_CHECK_DEPENDENCY:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will check neccesary lib and device.\n", __func__);
			for(i = 0; i < MAX_FILE_NUM; i++) {
				ret = access(file_check_list[i].strFilePath, file_check_list[i].mode);
				if(ret != 0) {
					output_debug_log(true, "[FM1388_CommandLine_Parser--%s], file (%s) does not exist or you have not enough right.\n", __func__, file_check_list[i].strFilePath);
					generate_fail_adb_result(result_filepath);
					break;
				}
			}
			if(i == MAX_FILE_NUM) {
				result_val[0] = 1;
				memset(result_str[0], 0, COMMENT_LENGTH);
				strncpy(result_str[0], SUCCESS, COMMENT_LENGTH - 1);
				ret = generate_adb_result(result_filepath, 1, result_val);
			}
			
			break;
			
		case FM1388_ADB_READ_WRITE_DATA:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will read/write data from momery.\n", __func__);
			ret = process_read_write_data(request_parameter_filepath, result_filepath);
			break;
			
		case FM1388_ADB_READ_WRITE_PARAMETER:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will read/write parameters.\n", __func__);
			ret = process_read_write_data(request_parameter_filepath, result_filepath);
			break;
			
		case FM1388_ADB_PLAY_RECORD:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will execute play/record.\n", __func__);
			memset(&st_play_record, 0, sizeof(SPIPlayRecord));
			st_play_record.cPlayMode = '0';
			st_play_record.cPlayOutput = '0';
			
			ret = get_play_record_parameters(argc, argv, &st_play_record);
			if(ret < 0) {
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], got wrong play/record command.\n", __func__);
				break;
			}
		
			/*			
			output_debug_log(true, "[%s] st_play_record.cOperation = %c\n", __func__, st_play_record.cOperation);
			output_debug_log(true, "[%s] st_play_record.cCommand = %c\n", __func__, st_play_record.cCommand);
			output_debug_log(true, "[%s] st_play_record.cChannelNum = %d\n", __func__, st_play_record.cChannelNum);
			output_debug_log(true, "[%s] st_play_record.strChannelMapping = %s\n", __func__, st_play_record.strChannelMapping);
			output_debug_log(true, "[%s] st_play_record.strInputFilePath = %s\n", __func__, st_play_record.strInputFilePath);
			output_debug_log(true, "[%s] st_play_record.strChannelIndex = %s\n", __func__, st_play_record.strChannelIndex);
			output_debug_log(true, "[%s] st_play_record.strOutputFilePath = %s\n", __func__, st_play_record.strOutputFilePath);
			output_debug_log(true, "[%s] st_play_record.cPlayMode = %c\n", __func__, st_play_record.cPlayMode);
			output_debug_log(true, "[%s] st_play_record.cPlayOutput = %c\n", __func__, st_play_record.cPlayOutput);
			*/	
			
			
			if(st_play_record.cOperation == SPIPLAY) {
				if((st_play_record.strChannelMapping[0] == 0) || (st_play_record.strInputFilePath[0] == 0)) {
					output_debug_log(true, "[FM1388_CommandLine_Parser--%s], You must provide the playback wav file path and channel mapping.\n", __func__);
					ret = -EPARAMINVAL;
				}
				else {
					snprintf(strParamemters, MAX_CMDLINE_LEN, "%c%c%x%s%c%s%c%c%c", st_play_record.cOperation, st_play_record.cCommand, st_play_record.cChannelNum, 
								st_play_record.strChannelMapping, PLACEHOLDER, st_play_record.strInputFilePath, PLACEHOLDER, st_play_record.cPlayMode, st_play_record.cPlayOutput);				
				}
			}
			else if(st_play_record.cOperation == SPIRECORD) {
				if((st_play_record.strChannelIndex[0] == 0) || (st_play_record.strOutputFilePath[0] == 0)) {
					output_debug_log(true, "[FM1388_CommandLine_Parser--%s], You must provide the recording wav file path and channel info.\n", __func__);
					ret = -EPARAMINVAL;
				}
				else {
					snprintf(strParamemters, MAX_CMDLINE_LEN, "%c%c%s%s%c", st_play_record.cOperation, st_play_record.cCommand, st_play_record.strChannelIndex, 
							st_play_record.strOutputFilePath, PLACEHOLDER);				
				}
			}
			else if(st_play_record.cOperation == SPIPLAYRECORD) {
				if((st_play_record.strChannelMapping[0] == 0) || (st_play_record.strInputFilePath[0] == 0) ||
					(st_play_record.strChannelIndex[0] == 0) || (st_play_record.strOutputFilePath[0] == 0)) {
					output_debug_log(true, "[FM1388_CommandLine_Parser--%s], You must provide the playback wav file path, channel mapping, recording wav file path and channel info.\n", __func__);
					ret = -EPARAMINVAL;
				}
				else {
					snprintf(strParamemters, MAX_CMDLINE_LEN, "%c%c%x%s%c%s%c%s%s%c%c%c", st_play_record.cOperation, st_play_record.cCommand, st_play_record.cChannelNum, 
							st_play_record.strChannelMapping, PLACEHOLDER, st_play_record.strInputFilePath, PLACEHOLDER, st_play_record.strChannelIndex, 
							st_play_record.strOutputFilePath, PLACEHOLDER, st_play_record.cPlayMode, st_play_record.cPlayOutput);				
				}
			}
			else {
				ret = -EPARAMINVAL;
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], got wrong play/record command.\n", __func__);
			}
			
			if(ret == ESUCCESS) {
				output_debug_log(true, "[%s] final command line=%s\n", __func__, strParamemters);
				ret = process_spi_operation(strParamemters, result_filepath, sdcard_path);
			}
			
			if (ret < 0) {
				generate_fail_adb_result(result_filepath);
				output_debug_log(true, "[FM1388_CommandLine_Parser] Error occurs when processing SPI play and record.\n");
			}
			break;
			
		case FM1388_ADB_SWITCH_MODE:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will switch DSP mode.\n", __func__);
			mode = -1;
			ret = get_mode_parameter(argc, argv, &mode);
			if((ret < 0) || (mode < 0)) {
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], got wrong switch mode command.\n", __func__);
				generate_fail_adb_result(result_filepath);
				break;
			}
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], mode=%d\n", __func__, mode);
			
			ret = set_mode(mode);
			if(ret < 0) {
				output_debug_log(true, "[FM1388_CommandLine_Parser--%s], Failed to switch mode.\n", __func__);
				generate_fail_adb_result(result_filepath);
				break;
			}
			
			result_val[0] = 1;
			memset(result_str[0], 0, COMMENT_LENGTH);
			strncpy(result_str[0], SUCCESS, COMMENT_LENGTH - 1);
			ret = generate_adb_result(result_filepath, 1, result_val);
			
			break;
			
		case FM1388_ADB_GET_FW_BUILD_NO:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will firmware build number.\n", __func__);
			break;
			
		case FM1388_ADB_DOWNLOAD_FIRMWARE:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will download firmware and set specified mode.\n", __func__);
			break;
			
		case FM1388_ADB_RESET_DSP:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], will reset DSP.\n", __func__);
			break;
			
		default:
			output_debug_log(true, "[FM1388_CommandLine_Parser--%s], unsupported command.\n", __func__);
			break;
			
	}
	
	return ret;
}
