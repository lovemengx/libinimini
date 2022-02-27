/**
******************************************************************************
* @文件	 libinimini.c
* @版本	 V1.0.1
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
#include <string.h>
#include "libinimini.h"

/********************************************************************
*	函数: 		libinimini_is_comment
*	功能:		判断字符串是否为注解
*	参数:		buf:字符串	len:字符串长度
*	返回:		0:不是 ini 的注释文本  1:为 ini 的注释文本  			
*********************************************************************/
static inline unsigned int libinimini_is_comment(char* buf, unsigned int len)
{
	return ';' == buf[0] ? 1 : 0;
}

/********************************************************************
*	函数: 		libinimini_is_section
*	功能:		判断字符串是否为字段类型
*	参数:		buf:字符串	len:字符串长度
*	返回:		0:不是 ini 的字段文本  1:为 ini 的字段文本
*********************************************************************/
static inline unsigned int libinimini_is_section(char* buf, unsigned int len)
{
	return '[' == buf[0] ? 1 : 0;
}

/********************************************************************
*	函数: 		libinimini_is_section
*	功能:		移除字符串的前后空格或制表符号
*	参数:		buf:字符串	len:字符串长度
*	返回:		处理后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_remove_frontback_space(char* buf, unsigned int len)
{
	char* p = buf;
	unsigned int cnt = 0;

	while (' ' == *p || '\t' == *p){
		cnt++, p++;
	}
	
	if ((len = len - cnt) > 0) {
		if (cnt) {
			memcpy(buf, buf + cnt, len);
		}
		p = buf + (len - 1);
		while (' ' == *p || '\t' == *p){
			len--, p--;
		}
	}
	buf[len] = '\0';

	return len;
}

/********************************************************************
*	函数: 		libinimini_terminator_stringt
*	功能:		寻找指定字符并在该处截断字符串
*	参数:		buf:字符串	len:字符串长度
*	返回:		0:未找到  >0:已找到并截断后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_terminator_stringt(char ech, char* buf, unsigned int len)
{
	char* k1 = NULL;
	if ((k1 = strchr(buf, ech)) != NULL) {
		*k1 = '\0';
		return (unsigned int)(k1 - buf);
	}
	return 0;
}

/********************************************************************
*	函数: 		libinimini_terminator_linefeed
*	功能:		截取换行符之前的字符串
*	参数:		buf:字符串	len:字符串长度
*	返回:		处理后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_terminator_linefeed(char* buf, unsigned int len)
{
	if((len = libinimini_terminator_stringt('\n', buf, len)) > 0){
		if('\r' == buf[len-1]){
			buf[len-1] = '\0';
			len = len - 1;
		}
	}

	return len;
}

/********************************************************************
*	函数: 		libinimini_extract_string
*	功能:		提取在 sch 字符和 ech 字符之间的字符串
*	参数:		sch:起始字符  ech:结束字符  buf:字符串  len:字符串长度
*	返回:		0:未找到  >0:提取成功后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_extract_string(char sch, char ech, char* buf, unsigned int len)
{
	char* k1 = NULL;
	char* k2 = NULL;

	if ((k1 = strchr(buf, sch)) == NULL || (k2 = strchr(k1 + 1, ech)) == NULL) {
		return 0;
	}

	if ((len = (unsigned int)(k2 - k1 - 1)) > 0) {
		memcpy(buf, k1 + 1, len);
	}
	buf[len] = '\0';
	return len;
}

/********************************************************************
*	函数: 		libinimini_find_strval
*	功能:		提取键的值
*	参数:		buf:字符串  len:字符串长度
*	返回:		提取成功后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_find_strval(char* buf, unsigned int len)
{
	len = libinimini_remove_frontback_space(buf, len);
	if (len && '\"' == buf[0] && '\"' == buf[len - 1]) {
		len = libinimini_extract_string('\"', '\"', buf, len);
		len = libinimini_remove_frontback_space(buf, len);
	}
	return len;
}

/********************************************************************
*	函数: 		libinimini_find_keyname
*	功能:		提取键名
*	参数:		buf:字符串  len:字符串长度  valpos:返回键值的起始位置
*	返回:		0:未找到键名  >0: 提取成功后的字符串长度以及键值的起始位置
*********************************************************************/
static inline unsigned int libinimini_find_keyname(char* buf, unsigned int len, unsigned int* valpos)
{
	if ((len = libinimini_terminator_stringt('=', buf, len)) <= 0) {
		return 0;
	}

	*valpos = len;
	len = libinimini_remove_frontback_space(buf, len);
	return len;
}

/********************************************************************
*	函数: 		libinimini_find_section
*	功能:		提取字段
*	参数:		buf:字符串  len:字符串长度
*	返回:		0:未找到字段  >0:提取成功后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_find_section(char* buf, unsigned int len)
{
	if ((len = libinimini_extract_string('[', ']', buf, len)) <= 0) {
		return 0;
	}
	len = libinimini_remove_frontback_space(buf, len);
	return len;
}

/********************************************************************
*	函数: 		libinimini_pre_process
*	功能:		对原始字符串的预处理,如移除换行符和空格、检查释放为注释
*	参数:		buf:字符串  len:字符串长度
*	返回:		0:预处理失败或是注释文本  >0:返回处理后的字符串长度
*********************************************************************/
static inline unsigned int libinimini_pre_process(char* buf, unsigned int len)
{
	if ((len = libinimini_terminator_linefeed(buf, len)) == 0) {
		return 0;
	}
	
	if ((len = libinimini_remove_frontback_space(buf, len)) == 0) {
		return 0;
	}

	if (libinimini_is_comment(buf, len) > 0) {
		return 0;
	}

	return len;
}

/********************************************************************
*	函数: 		libinimini_foreach
*	功能:		遍历整个 ini 配置文件, 并通过回调获取数据和输出内容
*	参数:		para: 参数配置
*				cache:用于处理数据和返回内容的缓冲区
*				size: 缓冲区大小, 必须大于字段名称+键值内容长度
*	返回:		已遍历成功的 ini 配置数量
*********************************************************************/
int libinimini_foreach(libinimini_parameter_t* para, char* cache, unsigned int size)
{
	char* buf = cache;
	libinimini_data_t data;
	unsigned int buflen = size, valpos = 0, len = 0, datacnt = 0;

	memset(&data, 0x00, sizeof(libinimini_data_t));
	while ((len = para->ops.getline_cb(buf, buflen, para->contex)) > 0)
	{
		// 对数据进行预处理
		if ((len = libinimini_pre_process(buf, len)) <= 0) {
			continue;
		}

		// 判断是否为段落
		if (libinimini_is_section(buf, len) > 0) {
			if ((len = libinimini_find_section(buf, len)) > 0) {
				if (data.section != NULL) {
					memcpy(cache, buf, len);
					cache[len] = '\0';
					buf = cache;
					buflen = size;
				}
				data.section = buf;
				buf = buf + len + 1;
				buflen = buflen - len - 1;
			}
			continue;
		}

		// 尝试提取键值对
		if (data.section && libinimini_find_keyname(buf, len, &valpos) > 0) {
			if ((len = libinimini_find_strval(buf + valpos + 1, len - valpos - 1)) > 0) {
				datacnt++;
				data.keyname = buf;
				data.strval = buf + valpos + 1;
				if (para->result(&data, para->contex) != LIB_INIMINI_KEEP) {
					break;
				}
			}
		}
	}

	return datacnt;
}
