/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : at.h
* Description: at 驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#ifndef __AT_H__
#define __AT_H__

/* Macros -----------------------------------------------*/
#define AT_UNINITED	0xf0	
#define AT_INITED	0xfd
#define AT_CMD_NAME_MAX_LEN   32

#define AT_RD       (0x01<<0)//可读
#define AT_WR       (0x01<<1)//可写
#define AT_EX       (0x01<<2)//可执行
#define AT_ERR		(0xff)

typedef struct _AT_CMD_T
{
    char            pcName[AT_CMD_NAME_MAX_LEN];
	uint8_t         ucType;
    const char *    pcParam;
    void       *    arAddr[10];
}at_cmd_t;

typedef int (*at_func)(uint8_t, at_cmd_t*, void*);

typedef struct __C_AT c_at_t;
typedef struct __C_AT
{
	uint8_t (*init)(uint8_t, uint32_t);
	uint8_t (*regist_cmd)(at_cmd_t *, at_func, void*);
}c_at_t;

/************************************************* 
* Demo:  
//@step1: 定义AT命令列表及命令列表回调函数
//@note: 支持格式制类型: %c,%d,%hd,%u,%hu,%f,%s
int 		xTemp;
float 		fTemp;
uint32_t 	uxTemp;
char 		arTemp[32] = "ASDQASDSADSWEZXC";
static at_cmd_t at_list[] = 
{
	{"AT+TIME", 	AT_RD|AT_WR|AT_EX, 		"%d,%u,%f,%s", &xTemp, &uxTemp, &fTemp, arTemp},
	{"AT+IP", 		AT_RD|AT_WR|AT_EX, 	"%d,%u,%f,%s", &xTemp, &uxTemp, &fTemp, arTemp},
	{"AT+PORT", 	AT_RD|AT_WR|AT_EX, 	"%d,%u,%f,%s", &xTemp, &uxTemp, &fTemp, arTemp},
	{NULL}
};
//回调函数
int at_execute_proc(uint8_t type, at_cmd_t* pxCmd, void *pvParam)
{
	log_debug("ex ok .");
	return E_OK;		//此返回即, AT模块的返回
}

//@step2: AT初始化
int ret;
ret =  s_theAT.init(MY_UART_1, 115200);
if(E_OK != ret)
{
	log_debug("at init failed .");
	return;
}

//@step3: 注册AT命令列表
ret = s_theAT.regist_cmd(at_list, at_execute_proc, NULL);
if(E_OK != ret)
{
	log_debug("at regist failed .");
	return;
}
*************************************************/
extern const c_at_t s_theAT;

#endif

