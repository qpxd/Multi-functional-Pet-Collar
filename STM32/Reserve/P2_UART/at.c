/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : at_uart.c
* Description: at_uart 串口AT模块
* Author: wyr
* Version: v1.0.2
* Date: 2023/02/16
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_AT_ENABLED

/* Macros -----------------------------------------------*/
#define R_OK  0
#define R_ERR 1


#define AT_REC_LEN              1500
#define AT_LIST_MAX             10          //允许注册的最大AT命令列表数量

#ifdef HELP_LOG_ENABLED
	#define LOG_OUT				log_debug
#else
	#define LOG_OUT
#endif

#ifdef SYSTEM_FREERTOS_ENABLED
	#define A_MALLOC			pvPortMalloc
	#define A_FREE				vPortFree
#else
	#define A_MALLOC			malloc
	#define A_FREE				free
#endif

/* Variables-----------------------------------------------*/
typedef struct _M_AT_T
{
	uint8_t ucState;
    uint8_t ucUartId;
    uint8_t ucListCnt;
    void*   arList[AT_LIST_MAX];
	at_func arFunc[AT_LIST_MAX];
    void*   arParam[AT_LIST_MAX];
}m_at_t;

/* Public function prototypes---------------------------------------------------*/
void prvDataProc(char *pcRec, at_cmd_t *pxCmd, uint8_t ucListId);
uint8_t xAtInit(uint8_t uart_id, uint32_t baud_rate);
uint8_t xAtRegistCmdList(at_cmd_t *pxList, at_func at_cb, void *);

/* Variables-----------------------------------------------*/
const c_at_t s_theAT = {xAtInit, xAtRegistCmdList};
static m_at_t m_this;

/* Public function prototypes---------------------------------------------------*/
static int m_callback(void* param, const unsigned char* data, unsigned short data_len)
{
    int ret = 0;
    static char arBuf[AT_REC_LEN];
	static char arTemp[AT_REC_LEN];
    static uint16_t ucLen;
    void* ptr = 0;
   

    /*参数检查*/
    if(NULL == param || NULL == data)
    {
        LOG_OUT("Null pointer.");
        return R_ERR;
    }
    if(0 == data_len)
    {
        LOG_OUT("Param error.");
        return R_ERR;
    }
	
    /*判断数据是否溢出*/
    if(ucLen + data_len > AT_REC_LEN)
    {
        if(data_len > AT_REC_LEN){
           LOG_OUT("Buffer over flow ."); 
            return R_ERR;
        }
        LOG_OUT("Out of lenth, data lose .");
        ucLen = 0;
    }

    /*数据处理*/
    memcpy(arBuf + ucLen, data, data_len);
    
    /*这里过滤掉16进制数据0x00，防止后面字符串处理失败*/
    for(uint16_t i = 0; i < data_len; i++)
    {
        if(arBuf[ucLen + i] == 0)
        {
            arBuf[ucLen + i] = ' ';
        }
    }
    ucLen += data_len;
    char *pxIndex;
    at_cmd_t *pxList;
    while(1)
    {
		pxIndex = strstr(arBuf, "\r\n");
		if(pxIndex == NULL)	break;
		pxIndex[0] = 0;
		pxIndex[1] = 0;
		memset(arTemp, 0, sizeof(arTemp));
		strcpy(arTemp, arBuf);
		
        for(uint8_t i = 0; i <  m_this.ucListCnt; i++)
        {
            if(m_this.arList[i] == NULL)   break;
            pxList = (at_cmd_t*)(m_this.arList[i]);
            while(strlen(pxList->pcName) != NULL)
            {
				prvDataProc(arTemp, pxList, i);         //收到结束标志 \r\n进入命令处理函数
                pxList++;
            }
        }
		
		ucLen = ucLen - (pxIndex - arBuf + 2);
		if(ucLen > 0){
			memset(arTemp, 0, sizeof(arTemp));
			memcpy(arTemp, pxIndex+2, ucLen);
			memset(arBuf, 0, sizeof(arBuf));
			memcpy(arBuf, arTemp, ucLen);
		}else{
			memset(arBuf, 0, sizeof(arBuf));
		}
    }
    return E_OK;
}



uint8_t xAtInit(uint8_t uart_id, uint32_t baud_rate)
{
    int ret = 0;
	if(m_this.ucState == AT_INITED)
	{
		LOG_OUT("AT already inited . ");
		return R_OK;
	}
    /*私有成员初始化*/
    m_this.ucUartId    	= uart_id;
    m_this.ucListCnt   	= 0;
    memset(m_this.arList, 0, sizeof(m_this.arList));
	memset(m_this.arFunc, 0, sizeof(m_this.arFunc));
	memset(m_this.arParam, 0, sizeof(m_this.arParam));
    /*初始化串口*/
    ret = my_uart.init(uart_id, baud_rate, 128, UART_MODE_DMA);
    if(E_OK != ret)
    {
        LOG_OUT("Uart init failed.");
        return R_ERR;
    }
    /*注册串口回调*/
    ret = my_uart.set_callback(uart_id, (void*)(&m_this), m_callback);
    if(E_OK != ret)
    {
        LOG_OUT("Callback set failed.");
       return R_ERR;
    }
	m_this.ucState	= AT_INITED;
    return R_OK;
}

uint8_t xAtRegistCmdList(at_cmd_t *pxList, at_func at_cb, void *pvParam)
{
    /*参数检查*/
    if(NULL == pxList)
    {
        LOG_OUT("Null pointer.");
        return R_ERR;
    }
    if(AT_INITED != m_this.ucState)
    {
        LOG_OUT("At not inited .");
        return R_ERR;
    }
    /*AT命令列表已满，无法注册新的命令列表*/
    if(m_this.ucListCnt >= AT_LIST_MAX)
    {
        LOG_OUT("AT command list is full .");
        return R_ERR;
    }
    /*注册新的命令列表 和 命令列表对应的回调函数*/
    m_this.arList[m_this.ucListCnt]     = (void*)pxList;
	m_this.arFunc[m_this.ucListCnt]	    = at_cb;
    m_this.arParam[m_this.ucListCnt]	= pvParam;
    m_this.ucListCnt++;
    /**/
    return R_OK;   
}


/* Private function prototypes---------------------------------------------------*/
void prvDataProc(char *pcRec, at_cmd_t *pxCmd, uint8_t ucListId)
{
    int ret = E_OK;
    static char pxArg[32];
    static char pxTxBuf[AT_REC_LEN];
	static char arStr[16];
	char *pxIndex = NULL;
    char *pcTemp;

	pcTemp = strstr(pcRec, pxCmd->pcName);
	if(pcTemp == NULL){
		return;
	}	
	// if(*(pcTemp-3) != 'A' || *(pcTemp-2) != 'T' || *(pcTemp-1) != '+'){
	// 	return;
	// }

	uint8_t type = AT_ERR;
	memset(pxArg, 0, sizeof(pxArg));
	memset(pxTxBuf, 0, sizeof(pxTxBuf));
	strcpy(pxArg, pxCmd->pcParam);
	
	pcTemp += strlen(pxCmd->pcName);	//指针后移指向命令末尾
	if(*pcTemp == '=' && (pxCmd->ucType&AT_WR) != 0){
		type = AT_WR;	/*设置命令*/
	}else if(*pcTemp == '?' && (pxCmd->ucType&AT_RD) != 0){
		type = AT_RD;	/*获取命令*/
	}else if(*pcTemp == '\0' && (pxCmd->ucType&AT_EX) != 0){
		type = AT_EX;	/*执行命令*/
	}else{
		return;	/*命令类型错误直接返回*/
	}	
	
    /*调用回调函数*/
    if(m_this.arFunc[ucListId] != NULL && type != AT_WR)
    {
        ret = m_this.arFunc[ucListId](type, pxCmd, m_this.arParam[ucListId]);	
    }
    	
	if(type == AT_RD)
	{
		strcat(pxTxBuf, "+");
		strcat(pxTxBuf, pxCmd->pcName);
		strcat(pxTxBuf, ":");
	}
	else if(type == AT_WR)
	{
		pxIndex = strtok(pcRec, "=");
	}

	for(uint8_t i = 0; i < 10; i++)
	{
		if(i == 0){
			pcTemp = pxArg;
		}else{
			pcTemp = strchr(pcTemp, ',');
			if(pcTemp == NULL)	break;
			pcTemp++;
		}

		if(type == AT_RD)				/*读取命令*/
		{
			memset(arStr, 0, sizeof(arStr));
			if(i != 0) strcat(pxTxBuf, ",");
			if(pcTemp[1] == 'd')
				sprintf(arStr, "%d", *(int*)(pxCmd->arAddr[i]));
			else if(pcTemp[1] == 'u')
				sprintf(arStr, "%u", *(uint32_t*)(pxCmd->arAddr[i]));
			else if(pcTemp[1] == 'f')
				sprintf(arStr, "%.4f", *(float*)(pxCmd->arAddr[i]));
            else if(pcTemp[1] == 'c')
				sprintf(arStr, "%c", *(char*)(pxCmd->arAddr[i]));
            else if(pcTemp[1] == 'h' && pcTemp[2] == 'd')
				sprintf(arStr, "%hd", *(short*)(pxCmd->arAddr[i]));
            else if(pcTemp[1] == 'h' && pcTemp[2] == 'u')
				sprintf(arStr, "%hu", *(unsigned short*)(pxCmd->arAddr[i]));
			else if(pcTemp[1] == 's')
			{
				char *pcTxStr = A_MALLOC(AT_REC_LEN);
				if(pcTxStr == NULL)
				{
					LOG_OUT("Memory request failed .");
					return;
				}
				memset(pcTxStr, 0, AT_REC_LEN);
				strcpy(pcTxStr, (char*)(pxCmd->arAddr[i]));
				pxIndex = strtok(pcTxStr, "\n");
				while(pxIndex != NULL)
				{
					strcat(pxTxBuf, pxIndex);
					pxIndex = strtok(NULL, "\n");
					if(pxIndex != NULL)
					{
						strcat(pxTxBuf, "\n+");
						strcat(pxTxBuf, pxCmd->pcName);
						strcat(pxTxBuf, ":");
					}
				}
				A_FREE(pcTxStr);
			}
			else
				LOG_OUT("Error type.");	
			strcat(pxTxBuf, arStr);
		}
		else if(type == AT_WR)			/*写入命令*/
		{
			pxIndex = strtok(NULL, ",");
			
			if(pxIndex == NULL){
				LOG_OUT("Data error .");
				return;
			}else{
				taskENTER_CRITICAL();
				if(pcTemp[1] == 'd')
					*(int*)(pxCmd->arAddr[i])            = atoi(pxIndex);
				else if(pcTemp[1] == 'u')
					*(uint32_t*)(pxCmd->arAddr[i])       = atoi(pxIndex);
				else if(pcTemp[1] == 'f')
					*(float*)(pxCmd->arAddr[i])          = atof(pxIndex);
                else if(pcTemp[1] == 'c')
					*(char*)(pxCmd->arAddr[i])           = atoi(pxIndex);
                else if(pcTemp[1] == 'h' && pcTemp[2] == 'd')
					*(short*)(pxCmd->arAddr[i])          = atoi(pxIndex);
                else if(pcTemp[1] == 'h' && pcTemp[2] == 'u')
					*(unsigned short*)(pxCmd->arAddr[i]) = atoi(pxIndex);
				else if(pcTemp[1] == 's')
					strcpy((char*)(pxCmd->arAddr[i]), pxIndex);
				else
					LOG_OUT("Error type.");
				taskEXIT_CRITICAL();
			}
		}
	}
	
    /*调用回调函数*/
    if(m_this.arFunc[ucListId] != NULL && type == AT_WR)
    {
        ret = m_this.arFunc[ucListId](type, pxCmd, m_this.arParam[ucListId]);	
    }
end:	
	if(type == AT_RD) strcat(pxTxBuf, "\r\n");
	if(ret == E_OK) strcat(pxTxBuf, "OK\r\n");
    else            strcat(pxTxBuf, "ERROR\r\n");
	if(my_uart.send(m_this.ucUartId, pxTxBuf, strlen(pxTxBuf)) != E_OK)
	{
		LOG_OUT("At uart send failed. ");
	}
}
#endif
