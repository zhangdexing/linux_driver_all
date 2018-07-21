#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../FM1388_Parameter_Lib/libFM1388Parameter.h"
#include "../libfm1388/libfm1388.h"


int process_spi_operation(char* parameter_string, char* result_file_path, char* strSDCARD) {
	SPIRecord		st_spi_record;
	SPIPlay			st_spi_play;
	SPIPlayRecord	st_spi_play_record;
	RequestPara		result_info;
	char			cOperation 	= -1;
	int				ret			= 0;
	int				rec_ch_num	= 0;
	int				i			= 0;
	char 			error_info[COMMENT_LENGTH] = { 0 };
	

	if (strSDCARD == NULL) {
		LOGD("[%s] wrong parameter, strSDCARD is NULL.\n", __func__);
		strcpy(error_info, "strSDCARD is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}

	if(parameter_string == NULL) {
		LOGD("[%s] parameter_string is null.\n", __func__);
		strcpy(error_info, "parameter_string is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}
	
	if(result_file_path == NULL) {
		LOGD("[%s] result_file_path is null.\n", __func__);
		strcpy(error_info, "result_file_path is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}
LOGD("[%s] parameter_string is %s.\n", __func__, parameter_string);
LOGD("[%s] result_file_path is %s.\n", __func__, result_file_path);
	

	cOperation = parameter_string[0];
	switch(cOperation) {
		case SPIPLAY:
			ret = parse_play_command(parameter_string, &st_spi_play);
			if(ret != ESUCCESS) {
				LOGD("[%s] Fail to parse SPI playback command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI playback command parameter");
				goto GEN_RESULT;
			}
			
			if(st_spi_play.cCommand == START) {
				//call libfm1388.so to do the real work
				ret = start_spi_playback_by_ADBTool(st_spi_play.strChannelMapping, 
						st_spi_play.strInputFilePath, st_spi_play.cPlayMode, st_spi_play.cPlayOutput);
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to start SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI playback");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play.cCommand == STOP) {
				//call libfm1388.so to do the real work
				ret = stop_spi_playback();
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to stop SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI playback");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play.cCommand == QUERY) {
				//call libfm1388.so to do the real work
				ret = is_playing();
				if(ret < 0) {
					LOGD("[%s] Failed to query SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to query SPI playback");
					goto GEN_RESULT;
				}
				else {
					if(ret == 1) {
						strcpy(error_info, "Playing");
					}
					else {
						strcpy(error_info, "Stopped");
					}
				}
			}
			else {
				LOGD("[%s] Got unsupported SPI playback command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI playback command");
				goto GEN_RESULT;
			}
			break;
			
		case SPIRECORD:
			ret = parse_record_command(parameter_string, &st_spi_record, strSDCARD);
			if(ret != ESUCCESS) {
				LOGD("[%s] Fail to parse SPI recording command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI recording command parameter");
				goto GEN_RESULT;
			}
			
			if(st_spi_record.cCommand == START) {
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}
				
				//call libfm1388.so to do the real work
				ret = start_debug_record(rec_ch_num, st_spi_record.strChannelIndex, st_spi_record.strOutputFilePath);
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to start SPI recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI recording");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_record.cCommand == STOP) {
				//call libfm1388.so to do the real work
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}

				ret = stop_debug_record_by_ADBTool(st_spi_record.strOutputFilePath, st_spi_record.strChannelIndex, rec_ch_num);
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to stop SPI recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI recording");
					goto GEN_RESULT;
				}
			}
			else {
				LOGD("[%s] Got unsupported SPI recording command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI recording command");
				goto GEN_RESULT;
			}
			break;
			
		case SPIPLAYRECORD:
			ret = parse_play_record_command(parameter_string, &st_spi_play_record, strSDCARD);
			if(ret != ESUCCESS) {
				LOGD("[%s] Fail to parse SPI playback & recording command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI playback & recording command parameter");
				goto GEN_RESULT;
			}

			if(st_spi_play_record.cCommand == START) {
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_play_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}
				
				//call libfm1388.so to do the real work
				ret = start_spi_playback_rec_by_ADBTool(st_spi_play_record.strChannelMapping, 
								st_spi_play_record.strInputFilePath, 1, rec_ch_num, 
								st_spi_play_record.strChannelIndex, 
								st_spi_play_record.strOutputFilePath,
								st_spi_play.cPlayMode, st_spi_play.cPlayOutput);
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to start SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI playback & recording");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play_record.cCommand == STOP) {
				//call libfm1388.so to do the real work
				ret = stop_spi_playback_rec(st_spi_play_record.strOutputFilePath, st_spi_play_record.strChannelIndex);
				if(ret != ESUCCESS) {
					LOGD("[%s] Failed to stop SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI playback & recording");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play_record.cCommand == QUERY) {
				//call libfm1388.so to do the real work
				ret = is_playing();
				if(ret < 0) {
					LOGD("[%s] Failed to query SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to query SPI playback & recording");
					goto GEN_RESULT;
				}
			}
			else {
				LOGD("[%s] Got unsupported SPI playback & recording command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI playback & recording command");
				goto GEN_RESULT;
			}
			break;
			
		default:
			LOGD("[%s] Unsupported operation.\n", __func__);
			ret = OPERATION_UNSUPPORTED;
			strcpy(error_info, "Unsupported operation");
			goto GEN_RESULT;
	}

GEN_RESULT:
	//write result to result file according to ret value
	result_info.op 		= 1;
	result_info.addr 	= 0xA5A5A5A5;
	result_info.value = ret;
	if(ret != ESUCCESS) {
		strncpy(result_info.comment, error_info, COMMENT_LENGTH);
	}
	else {
		if(strlen(error_info) != 0)
			strncpy(result_info.comment, error_info, COMMENT_LENGTH);
		else 
			strncpy(result_info.comment, "Success", COMMENT_LENGTH);
	}
		
	generate_result_file(result_file_path, &result_info, 1);

	return ret;
}
