/**
******************************************************************************
* @文件		main.c
* @版本		V1.0.0
* @日期
* @概要		用于举例说明 libinimini 如何在单片机中应用
* @作者		lmx
******************************************************************************
* @注意
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "fs/fatfs/ff.h"
#include "kernel/os/os.h"
#include "../src/libinimini.h"
#include "common/framework/fs_ctrl.h"
#include "common/framework/platform_init.h"

// 由 libinimini 回调，用于将解析的结果返回
// 返回值: LIB_INIMINI_STOP:停止解析  LIB_INIMINI_KEEP:继续解析
static int inimini_result_cb(libinimini_data_t* data, void* context)
{
	printf("section:%-20s keyname:%-30s strval:%-30s\n", data->section, data->keyname, data->strval);
	if (strcmp(data->section, "compass_para") == 0 && strcmp(data->keyname, "compass_int") == 0) {
		printf("-------------------------------------------------\n");
		printf("[%s]\n", data->section);
		printf("%s = %s\n", data->keyname, data->strval);
		printf("-------------------------------------------------\n");
		return LIB_INIMINI_STOP;
	}
	return LIB_INIMINI_KEEP;
}

// 由 libinimini 回调，用于获取每一行的原始文本数据
// 返回值: 0: 已无数据可以提供  >0: 字符串数据的长度 
static unsigned int inimini_getline_cb(char* buf, unsigned int size, void* context)
{
	FIL* fp = (FIL*)context;
	if (f_gets(buf, size, fp) == NULL) {
		return LIB_INIMINI_STOP;
	}
	return (unsigned int)strlen(buf);
}

int main(void)
{
	FIL file;
	static char cache[512] = { 0 };
	libinimini_parameter_t para;

	platform_init();
	
	if(fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != FS_MNT_STATUS_MOUNT_OK){
		printf("fs mount failed\n");
		while(1) OS_Sleep(1);
	}
	
	if(f_open(&file, "sys_config.ini", FA_READ | FA_OPEN_EXISTING) != FR_OK){
		printf("open file failed\n");
		while(1) OS_Sleep(1);
	}

	memset(&para, 0x00, sizeof(libinimini_parameter_t));
	para.contex = &file;
	para.result = inimini_result_cb;
	para.ops.getline_cb = inimini_getline_cb;
	libinimini_foreach(&para, cache, sizeof(cache));
	f_close(&file);
	
	printf("libinimini_foreach done...\n");
	while(1) OS_Sleep(1);
	return 0;
}
