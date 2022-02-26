## libinimini

最近自己基于 XR872 做一个小作品练习练习，具备可以配置的功能，选择了使用 ini 作为配置文件。我调研了网上常见的 ini 解析库，几乎都涉及到了 fopen()/fgets().. 以及 malloc()。

说明这些开源库都仅适用于 linux 系统，并不适用于 RTOS 或裸跑的单片机。因为前者虽是 C 语言的标准文件操作函数，但在单片机中基本上使用的都是简化版的 fatfs 接口，要想引入单片机使用，意味着需要对该接口库进行修改。后者更是涉及到内存管理，ram 的占用会随着 ini 配置文件的内容而变化，意味着 ram 的使用不可控，极易受外部因素影响，这对 ram 资源极为稀有的单片机来说，是不可接受的。

本着学习的态度，就设计了一个非常简单的 ini 配置文件解析库(libinimini)，具有以下几种特点：

* 内存空间占用可控，libinimini 只使用用户指定的一段内存空间进行解析和返回结果。
* 不关心数据的来源，libinimini 通过回调用户的接口获取每一行文本，不关心文本来自文件还是其它通信接口。
* 使用方便简单易上手，用户只需实现以行为单位的文本数据回调接口，之后只需等待 libinimini 解析结果即可。

注意：接口本身会将键值作为字符串传递出来，如果需要转换为数字，调用 atoi() 的函数转换即可。

## 接口介绍

```C
// 缓冲区回调
typedef struct {
	/*****************************************************************************
	*	函数: 	getline_cb
	*	功能:	由用户实现, 用于接口库获取 ini 配置文件的每一行字符串的函数
	*	参数:	buf:由接口库传入的缓冲区, 用于存储一行字符串  size:缓冲区大小
	*			contex:用户的上下文指针, 由初始化时用户传入
	*	返回:	0:已无数据可以提供,接口库会停止解析  >0:字符串数据的长度
	******************************************************************************/
	unsigned int (*getline_cb)(char* buf, unsigned int size, void* contex);
}libinimini_buffer_ops;

// ini 配置的某一项字段内容
typedef struct {
	const char* section;	// ini 字段字符串
	const char* keyname;	// ini 键名字符串
	const char* strval; 	// ini 键值字符串
}libinimini_data_t;

/*****************************************************************************
*	函数:		libinimini_result_fun
*	功能:		接口库解析到完整的配置信息后会通过此回调返回数据
*	参数:		data:接口库解析成功的配置信息
*				contex:用户的上下文指针, 由初始化时用户传入
*	返回:		LIB_INIMINI_KEEP:允许接口库继续解析下一个配置	
*				LIB_INIMINI_STOP:终止接口库继续解析配置
******************************************************************************/
typedef int (*libinimini_result_fun)(libinimini_data_t* data, void* contex);

// 接口参数配置
typedef struct {
	libinimini_buffer_ops ops;		// 获取数据的回调
	libinimini_result_fun result;	// 数据结果的回调
	void* contex;					// 用户上下文指针
}libinimini_parameter_t;

/********************************************************************
*	函数: 		libinimini_foreach
*	功能:		遍历整个 ini 配置文件, 并通过回调获取数据和输出内容
*	参数:		para: 参数配置
*				cache:用于处理数据和返回内容的缓冲区
*				size: 缓冲区大小, 必须大于字段名称+键值内容长度
*	返回:		已遍历成功的 ini 配置数量
*********************************************************************/
int libinimini_foreach(libinimini_parameter_t* para, char* cache, unsigned int size);
```

## sys_config.ini (示例配置文件)

![](/images/sys_config.png "sys config")


## Sample (FreeRTOS@xr872)

```C
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
```
![](/images/sample_xr872.png "xr872 example")

## Sample（Windows/Linux）

```C
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
	
```
![](/images/sample_windows_linux.png "windows linux example")