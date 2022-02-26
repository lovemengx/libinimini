/**
******************************************************************************
* @文件	 libinimini.c
* @版本	 V1.0.0
* @日期	 2022-02-26
* @概要
*		 1. 适用于在跑 RTOS 或裸跑的单片机平台上读取 ini 配置文件。
*		 2. 因单片机 RAM 稀有性,接口不会动态申请, 所需的内存由外部提供。
*		 3. 因单片机的文件系统差异和数据来源丰富, 接口实现不局限于文件。
* @作者	 lmx   QQ:1007566569   EMAIL:lovemengx@foxmail.com
******************************************************************************
* @注意
******************************************************************************
*/
#ifndef __LIB_INI_MINI_H__
#define __LIB_INI_MINI_H__

#define LIB_INIMINI_KEEP		1	// 继续解析
#define LIB_INIMINI_STOP		0	// 终止解析

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
	const char* strval;		// ini 键值字符串
}libinimini_data_t;

/*****************************************************************************
*	函数: 		libinimini_result_fun
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

#endif
