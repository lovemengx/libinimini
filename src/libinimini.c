/**
******************************************************************************
* @�ļ�	 libinimini.c
* @�汾	 V1.0.0
* @����	 2022-02-26
* @��Ҫ	 
*		 1. ���������� RTOS �����ܵĵ�Ƭ��ƽ̨�϶�ȡ ini �����ļ���
*		 2. ��Ƭ�� RAM ϡ����,�ӿڲ��ᶯ̬����, ������ڴ����ⲿ�ṩ��
*		 3. ��Ƭ�����ļ�ϵͳ�����������Դ�ḻ, �ӿ�ʵ�ֲ��������ļ���
* @����	 lmx   QQ:1007566569   EMAIL:lovemengx@foxmail.com
******************************************************************************
* @ע��
******************************************************************************
*/
#include <string.h>
#include "libinimini.h"

/********************************************************************
*	����: 		libinimini_is_comment
*	����:		�ж��ַ����Ƿ�Ϊע��
*	����:		buf:�ַ���	len:�ַ�������
*	����:		0:���� ini ��ע���ı�  1:Ϊ ini ��ע���ı�  			
*********************************************************************/
static inline unsigned int libinimini_is_comment(char* buf, unsigned int len)
{
	return ';' == buf[0] ? 1 : 0;
}

/********************************************************************
*	����: 		libinimini_is_section
*	����:		�ж��ַ����Ƿ�Ϊ�ֶ�����
*	����:		buf:�ַ���	len:�ַ�������
*	����:		0:���� ini ���ֶ��ı�  1:Ϊ ini ���ֶ��ı�
*********************************************************************/
static inline unsigned int libinimini_is_section(char* buf, unsigned int len)
{
	return '[' == buf[0] ? 1 : 0;
}

/********************************************************************
*	����: 		libinimini_is_section
*	����:		�Ƴ��ַ�����ǰ��ո���Ʊ����
*	����:		buf:�ַ���	len:�ַ�������
*	����:		�������ַ�������
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
*	����: 		libinimini_terminator_stringt
*	����:		Ѱ��ָ���ַ����ڸô��ض��ַ���
*	����:		buf:�ַ���	len:�ַ�������
*	����:		0:δ�ҵ�  >0:���ҵ����ضϺ���ַ�������
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
*	����: 		libinimini_extract_string
*	����:		��ȡ�� sch �ַ��� ech �ַ�֮����ַ���
*	����:		sch:��ʼ�ַ�  ech:�����ַ�  buf:�ַ���  len:�ַ�������
*	����:		0:δ�ҵ�  >0:��ȡ�ɹ�����ַ�������
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
*	����: 		libinimini_find_strval
*	����:		��ȡ����ֵ
*	����:		buf:�ַ���  len:�ַ�������
*	����:		��ȡ�ɹ�����ַ�������
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
*	����: 		libinimini_find_keyname
*	����:		��ȡ����
*	����:		buf:�ַ���  len:�ַ�������  valpos:���ؼ�ֵ����ʼλ��
*	����:		0:δ�ҵ�����  >0: ��ȡ�ɹ�����ַ��������Լ���ֵ����ʼλ��
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
*	����: 		libinimini_find_section
*	����:		��ȡ�ֶ�
*	����:		buf:�ַ���  len:�ַ�������
*	����:		0:δ�ҵ��ֶ�  >0:��ȡ�ɹ�����ַ�������
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
*	����: 		libinimini_pre_process
*	����:		��ԭʼ�ַ�����Ԥ����,���Ƴ����з��Ϳո񡢼���ͷ�Ϊע��
*	����:		buf:�ַ���  len:�ַ�������
*	����:		0:Ԥ����ʧ�ܻ���ע���ı�  >0:���ش������ַ�������
*********************************************************************/
static inline unsigned int libinimini_pre_process(char* buf, unsigned int len)
{
	unsigned int terlen = 0x00;
	
	if ((terlen = libinimini_terminator_stringt('\n', buf, len)) > 0){
		len = terlen;
	}
	
	if ((terlen = libinimini_terminator_stringt('\r', buf, len)) > 0){
		len = terlen;
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
*	����: 		libinimini_foreach
*	����:		�������� ini �����ļ�, ��ͨ���ص���ȡ���ݺ��������
*	����:		para: ��������
*				cache:���ڴ������ݺͷ������ݵĻ�����
*				size: ��������С, ��������ֶ�����+��ֵ���ݳ���
*	����:		�ѱ����ɹ��� ini ��������
*********************************************************************/
int libinimini_foreach(libinimini_parameter_t* para, char* cache, unsigned int size)
{
	char* buf = cache;
	libinimini_data_t data;
	unsigned int buflen = size, valpos = 0, len = 0, datacnt = 0;

	memset(&data, 0x00, sizeof(libinimini_data_t));
	while ((len = para->ops.getline_cb(buf, buflen, para->contex)) > 0)
	{
		// �����ݽ���Ԥ����
		if ((len = libinimini_pre_process(buf, len)) <= 0) {
			continue;
		}

		// �ж��Ƿ�Ϊ����
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

		// ������ȡ��ֵ��
		if (libinimini_find_keyname(buf, len, &valpos) > 0) {
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