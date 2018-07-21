#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../FM1388_Parameter_Lib/libFM1388Parameter.h"
#include "../libfm1388/libfm1388.h"

const char* mode_config_file_name = "FM1388_mode.cfg";
const char* request_parameter_filename				= "FM1388_ADB_Parameter.txt";
const char* result_filename							= "FM1388_ADB_Result.txt";
char request_parameter_filepath[MAX_PATH_LENGTH]	= { 0 };
char result_filepath[MAX_PATH_LENGTH]				= { 0 };

//parse mode config file
int parse_mode_config(char* firmware_path, ModeInfo** modeinfo, int* mode_num) {
	char	mode_config_file_path[MAX_PATH_LENGTH] = { 0 };
	int		num = 0;
	int		ret = 0;

	if ((firmware_path == NULL) || (mode_num == NULL)) {
		LOGD("[parse_mode_config] Got wrong parameter.\n");
		return -1;
	}

	snprintf(mode_config_file_path, MAX_PATH_LENGTH - 1, "%s%s", firmware_path, mode_config_file_name);

	num = get_parameter_number(mode_config_file_path);
	//LOGD("[parse_mode_config] num=%d\n", num);
	if (num <= 0) {
		LOGD("[parse_mode_config] Got empty or wrong mode information file.\n");
		return -1;
	}

	*modeinfo = (ModeInfo*)malloc(sizeof(ModeInfo) * num);
	if (*modeinfo == NULL) {
		LOGD("[parse_mode_config] Can not allocate memory for mode information.\n");
		return -1;
	}

	ret = parse_mode_file(mode_config_file_path, *modeinfo, ',');
	//LOGD("[parse_mode_config] parse_mode_file return=%d\n", ret);
	if (ret < 0) {
		LOGD("[parse_mode_config] Error occurs when parsing mode information.\n");
		return -1;
	}

	*mode_num = num;

	return 0;
}

int get_request_parameter(char* request_parameter_filepath, RequestPara** request_para, int* request_parameter_number) {
	int temp_parameter_number = 0;
	int parameter_number = 0;
	int i = 0;
	
	if ((request_parameter_filepath == NULL) || (request_parameter_number == NULL)) {
		LOGD("[get_request_parameter] Got wrong parameter.\n");
		return -1;
	}

	//get requested parameter number to allocate memory for structure
	temp_parameter_number = get_parameter_number(request_parameter_filepath);
	if (temp_parameter_number <= 0) {
		LOGD("[get_request_parameter] Got empty request parameter file.\n");
		return -1;
	}

	*request_para = (RequestPara*)malloc(sizeof(RequestPara) * temp_parameter_number);
	if (*request_para == NULL) {
		LOGD("[get_request_parameter] Can not allocate memory for request parameter.\n");
		return -1;
	}
	//LOGD("[get_request_parameter] get request parameter number=%d\n", temp_parameter_number);

	//parse requested parameter file
	parameter_number = parse_para_file(request_parameter_filepath, *request_para, '\t');
	if (parameter_number <= 0) {
		LOGD("[get_request_parameter] Got wrong format request parameter file.\n");
		return -1;
	}
	
	*request_parameter_number = parameter_number;
	
	return 0;
}

int main(int argc, char** argv)
{
	char			sdcard_path[MAX_PATH_LENGTH] = { 0 };
	char			firmware_path[MAX_PATH_LENGTH] = { 0 };
	RequestPara* 	request_parameter			= NULL;
	RequestPara* 	result_parameter			= NULL;
	int				request_parameter_number	= 0;
	int 			total_parameter_number		= 0;
	ModeInfo*		mode_info		= NULL;
	int				mode_number		= 0;
	int				i				= 0;
	int				result_index	= 0;
	int				need_change		= 0;
	int				ret				= 0;
	int				current_mode	= 0;
	char			current_parameter_file_path[MAX_PATH_LENGTH] = { 0 };

	LOGD("[FM1388_ADB_Tool] enter.\n");
	
	//initialize lifm1388 and device
	ret = lib_open_device();
	if (ret != ESUCCESS) {
		LOGD("[FM1388_ADB_Tool] FM1388 does not work normally.\n");
		return -1;
	}

	//get sdcard path from command line parameter
	if ((argc > 4)) {
		LOGD("[FM1388_ADB_Tool] Too many parameters.\n");
		goto END;
	}
	else if ((argc == 3)) {
		strncpy(sdcard_path, argv[1], MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, argv[2], MAX_PATH_LENGTH - 1);
	}
	else if ((argc == 4)) {
		strncpy(sdcard_path, argv[1], MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, argv[2], MAX_PATH_LENGTH - 1);
	}
	else {
		strncpy(sdcard_path, get_sdcard_path(), MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, get_cfg_location(), MAX_PATH_LENGTH - 1);
		
		//omit the file name in path
		for(i = strlen(firmware_path) - 1; i > 0; i--) {
			if(firmware_path[i] == '/') break;
		}
		if((i <= 0) || (firmware_path[i] != '/')) {
			LOGD("[FM1388_ADB_Tool] FM1388 firmware and parameter file path is not correct.\n");
			goto END;
		}
		else {
			firmware_path[i + 1] = 0;
		}	
	}

	//prepare request and result file path
	snprintf(request_parameter_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, request_parameter_filename);
	snprintf(result_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, result_filename);

	if(argc == 4) {
		ret = process_spi_operation(argv[3], result_filepath, sdcard_path);
		if (ret < 0) {
			LOGD("[FM1388_ADB_Tool] Error occurs when processing SPI play and record.\n");
		}
		goto END;
	}
	else {
		//parse mode config file
		ret = parse_mode_config(firmware_path, &mode_info, &mode_number);
		//LOGD("[FM1388_ADB_Tool] ret=%d\n", ret);
		//LOGD("[FM1388_ADB_Tool] mode_number=%d\n", mode_number);
		if (ret < 0) {
			LOGD("[FM1388_ADB_Tool] Error occurs when parsing mode information.\n");
			goto END;
		}

		//get current mode and vec file path
		current_mode = get_mode();
		//LOGD("[FM1388_ADB_Tool] current_mode=%d\n", current_mode);
		if (current_mode < 0) {
			LOGD("[FM1388_ADB_Tool] Error occurs when calling get_mode(). ret = %d\n", current_mode);
			goto END;
		}

		//LOGD("[FM1388_ADB_Tool] mode_info[current_mode].parameter_file_name=%s\n", mode_info[current_mode].parameter_file_name);

		//parse current performance parameter vec file to memory
		snprintf(current_parameter_file_path, MAX_PATH_LENGTH - 1, "%s%s", firmware_path, mode_info[current_mode].parameter_file_name);

		//get requested parameter number to allocate memory for structure
		ret = get_request_parameter(request_parameter_filepath, &request_parameter, &request_parameter_number);
		LOGD("[FM1388_ADB_Tool] ret=%d\n", ret);
		LOGD("[FM1388_ADB_Tool] request_parameter_number=%d\n", request_parameter_number);
		if (ret < 0) {
			LOGD("[FM1388_ADB_Tool] Error occurs when calling get_request_parameter(). ret=%d\n", ret);
			goto END;
		}


		LOGD("[FM1388_ADB_Tool] deal with write parameter.\n");
		//process write operation first and save parameter vec file
		for (i = 0; i < request_parameter_number; i++) {
			if (request_parameter[i].op == OPERATION_WRITE) {
				ret = set_dsp_mem_value_spi(request_parameter[i].addr, request_parameter[i].value);
				if(ret != ESUCCESS) {
					LOGD("[FM1388_ADB_Tool] Failed to write parameter to FM1388. ret=%d\n", ret);
				}
			}
		}

		LOGD("[FM1388_ADB_Tool] deal with read parameter.\n");
		//process read operation, then generate result file
		result_parameter = (RequestPara*)malloc(sizeof(RequestPara) * request_parameter_number);
		if (result_parameter == NULL) {
			LOGD("[FM1388_ADB_Tool] Can not allocate memory for result parameter.\n");
			goto END;
		}

		LOGD("[FM1388_ADB_Tool] generate result array.\n");
		result_index = 0;
		for (i = 0; i < request_parameter_number; i++) {
			if (request_parameter[i].op == OPERATION_READ) {
				request_parameter[i].value = get_dsp_mem_value_spi(request_parameter[i].addr) & 0xFFFF;
				
				result_parameter[result_index].addr = request_parameter[i].addr;
				result_parameter[result_index].value = request_parameter[i].value;
				LOGD("[FM1388_ADB_Tool] address=%08x\n", result_parameter[result_index].addr);
				LOGD("[FM1388_ADB_Tool] value=%04x\n", result_parameter[result_index].value);
				strncpy(result_parameter[result_index].comment, request_parameter[i].comment, COMMENT_LENGTH);

				result_index++;
			}
		}
		
		LOGD("[FM1388_ADB_Tool] generate result file.\n");		
		if(result_index > 0)
			generate_result_file(result_filepath, result_parameter, result_index);
	}
	
END:
	if (request_parameter != NULL) free(request_parameter);
	if (result_parameter != NULL) free(result_parameter);
	if (mode_info != NULL) free(mode_info);
	lib_close_device();

	LOGD("[FM1388_ADB_Tool] Finished!\n");

	return 0;
}
