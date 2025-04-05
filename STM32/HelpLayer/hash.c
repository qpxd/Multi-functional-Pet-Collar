/**
******************************************************************************
* File Name          : flash.c
* Description        : This file provides code for the configuration
*                      of all used GPIO pins.
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "help_conf.h"

#ifdef HELP_HASH_ENABLED

/* Macros -----------------------------------------------*/
#define R_OK  0
#define R_ERR 1

#define H_MALLOC    malloc
#define H_FREE      free
#define LOG_OUT
#define CMD_HASH 	0xb433e5c6
/* Variables-----------------------------------------------*/
typedef struct 
{
    hash_node_t *pxEnd;

}m_hash_t;

/* Private function declare-------------------------------------------------------------*/
static char prvCharToLower(char c);
static uint32_t prvHashCode(const char* str);

static uint8_t xHashAddStr(const c_hash_t *pxCTemp, char* pcKey, char* pcValue);
static uint8_t xHashAddArr(const c_hash_t *pxCTemp, char* pcKey, char* pcValue, uint16_t uxLen);

static uint8_t xHashFind(const c_hash_t *pxCTemp, char *pcKey);

/* Public function prototypes-------------------------------------------------------------*/
c_hash_t *hash_create(void)
{
    m_hash_t *pxMTemp;
    c_hash_t *pxCTemp;

    //1. 申请内存，私有成员
    pxMTemp = H_MALLOC(sizeof(m_hash_t));
    if(NULL == pxMTemp)
    {
        LOG_OUT("Memmory request failed.");
        return NULL;
    }
    memset(pxMTemp, 0, sizeof(m_hash_t));
    //2. 申请内存，公有成员
    pxCTemp = H_MALLOC(sizeof(c_hash_t));
    if(NULL == pxCTemp)
    {
        LOG_OUT("Memmory request failed.");
        return NULL;
    }
    memset(pxCTemp, 0, sizeof(c_hash_t));
    //3. 私有成员赋值
    pxMTemp->pxEnd =  H_MALLOC(sizeof(hash_node_t));
    if(NULL == pxMTemp->pxEnd)
    {
        LOG_OUT("hash: Memory request failed.");
        H_FREE(pxMTemp);
        H_FREE(pxCTemp);
        return NULL;
    }   
	
    pxMTemp->pxEnd->pcKey       = "END-KEY-0xb433e5c6";
    pxMTemp->pxEnd->pcValue     = "END-VALUE-0xb433e5c6";
    pxMTemp->pxEnd->uxHashCode  = prvHashCode(pxMTemp->pxEnd->pcKey);
    pxMTemp->pxEnd->pxNext      = pxMTemp->pxEnd;
    pxMTemp->pxEnd->pxPrevious  = pxMTemp->pxEnd;
	
    //4. 公有有成员赋值
	pxCTemp->this				= pxMTemp;
	pxCTemp->add_str			= xHashAddStr;
	pxCTemp->add_arr			= xHashAddArr;
	pxCTemp->find				= xHashFind;
	
	return pxCTemp;
}

static uint8_t xHashAddStr(const c_hash_t *pxCTemp, char* pcKey, char* pcValue)
{
    if(NULL == pxCTemp){
        LOG_OUT("Null pointer.");
        return R_ERR;
    }
    const m_hash_t *pxMTemp = (m_hash_t*)(pxCTemp->this);
	//在hash表中查询此键值是否存在
    hash_node_t *pxIndex;
    uint32_t uxHashCode = prvHashCode(pcKey);       //计算 键值Key对应的HASH值
    pxIndex = pxMTemp->pxEnd;                       //索引从根节点开始
	do{
		if(uxHashCode == pxIndex->uxHashCode)	break;
		pxIndex = pxIndex->pxNext;
	}while(pxIndex != pxMTemp->pxEnd);
	//hash表中未找到此键，新建键值对
	if(pxIndex == pxMTemp->pxEnd){	
		pxIndex = H_MALLOC(sizeof(hash_node_t));
		if(NULL == pxIndex)
		{
			LOG_OUT("Memory request failed.");
			return R_ERR;
		}
		pxIndex->pcKey                  = pcKey;
		pxIndex->pcValue                = pcValue;
		pxIndex->uxHashCode             = prvHashCode(pcKey);
		/*节点插入链表*/   
		pxIndex->pxPrevious                 = pxMTemp->pxEnd->pxPrevious;
		pxMTemp->pxEnd->pxPrevious->pxNext  = pxIndex;
		pxIndex->pxNext                     = pxMTemp->pxEnd;
		pxMTemp->pxEnd->pxPrevious          = pxIndex;
	
	}else{	    //hash表中找到了此键 直接覆盖其值
		pxIndex->pcValue = pcValue;
	}
    return R_OK;
}


static uint8_t xHashAddArr(const c_hash_t *pxCTemp, char* pcKey, char* pcValue, uint16_t uxLen)
{
    if(NULL == pxCTemp){
        LOG_OUT("Null pointer.");
        return R_ERR;
    }
    const m_hash_t *pxMTemp = (m_hash_t*)(pxCTemp->this);
	//在hash表中查询此键值是否存在
    hash_node_t *pxIndex;
    uint32_t uxHashCode = prvHashCode(pcKey);       //计算 键值Key对应的HASH值
    pxIndex = pxMTemp->pxEnd;                       //索引从根节点开始
	do{
		if(uxHashCode == pxIndex->uxHashCode)	break;
		pxIndex = pxIndex->pxNext;
	}while(pxIndex != pxMTemp->pxEnd);
	//hash表中未找到此键，新建键值对
	if(pxIndex == pxMTemp->pxEnd){	
		pxIndex = H_MALLOC(sizeof(hash_node_t));
		if(NULL == pxIndex)
		{
			LOG_OUT("Memory request failed.");
			return R_ERR;
		}
		pxIndex->pcKey                  = pcKey;
		memcpy(pcValue, pxIndex->pcValue, uxLen);
		pxIndex->uxHashCode             = prvHashCode(pcKey);
		/*节点插入链表*/   
		pxIndex->pxPrevious                 = pxMTemp->pxEnd->pxPrevious;
		pxMTemp->pxEnd->pxPrevious->pxNext  = pxIndex;
		pxIndex->pxNext                     = pxMTemp->pxEnd;
		pxMTemp->pxEnd->pxPrevious          = pxIndex;
	
	}else{	    //hash表中找到了此键 直接覆盖其值
		memcpy(pcValue, pxIndex->pcValue, uxLen);
	}
    return R_OK;
}


static uint8_t xHashFind(const c_hash_t *pxCTemp, char *pcKey)
{
    if(NULL == pxCTemp)
	{
        LOG_OUT("Null pointer.");
        return R_ERR;
    }
	
    const m_hash_t *pxMTemp = (m_hash_t*)(pxCTemp->this);
    hash_node_t *pxIndex = pxMTemp->pxEnd;
    uint32_t uxHashCode = prvHashCode(pcKey);
	do{
		if(uxHashCode == pxIndex->uxHashCode){
//			memcpy(pcValue, pxIndex->pcValue, uxLen);
			break;
		}
		pxIndex = pxIndex->pxNext;
	}while(pxIndex != pxMTemp->pxEnd);
	
    if(pxIndex == pxMTemp->pxEnd)
	{
        LOG_OUT("Key is not found.");
        return R_ERR;
    }
    return R_OK;
}

static char prvCharToLower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

static uint32_t prvHashCode(const char* str)
{
    int tmp, c = *str;
    unsigned int seed = CMD_HASH;  /* 'jiejie' string hash */  
    unsigned int hash = 0;
    
    while(*str) {
        tmp = prvCharToLower(c);
        hash = (hash ^ seed) + tmp;
        str++;
        c = *str;
    }
    return hash;
}

#endif

/* Public function prototypes-------------------------------------------------------------*/


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
