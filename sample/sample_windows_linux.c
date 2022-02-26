/**
******************************************************************************
* @文件		main.c
* @版本		V1.0.0
* @日期
* @概要		用于举例说明 libinimini 如何在 Windows/Linux 中应用
* @作者		lmx
******************************************************************************
* @注意
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "../src/libinimini.h"

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
static unsigned int inimini_getline_cb(char* buf, unsigned int size, void* contex)
{
	FILE* fp = (FILE*)contex;
	if (fgets(buf, size, fp) == NULL) {
		return LIB_INIMINI_STOP;
	}
	return strlen(buf);
}

int main(void)
{
	char inimini_cache[1024] = { 0 };
	libinimini_parameter_t para;

	FILE* fp = fopen("F:/sys_config.ini", "r");
	if (NULL == fp) {
		printf("open file failed\n");
		return 0;
	}

	memset(&para, 0x00, sizeof(libinimini_parameter_t));
	para.contex = fp;
	para.result = inimini_result_cb;
	para.ops.getline_cb = inimini_getline_cb;
	int cnt = libinimini_foreach(&para, inimini_cache, sizeof(inimini_cache));
	fclose(fp);

	printf("libinimini_foreach done...\n");
	return 0;
}
