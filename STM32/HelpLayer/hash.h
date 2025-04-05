/**
******************************************************************************
* File Name          : hash.h
* Description        : This file contains all the functions prototypes for 
*                      the fsmc  
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __hash_H
#define __hash_H

#ifdef __cplusplus
extern "C" {
#endif
  
/* typedef  -------------------------------------------------------------*/
typedef struct xHASH_NODE_T hash_node_t;
typedef struct xHASH_NODE_T 
{
	char*     pcKey;
	char*     pcValue;
	uint32_t        uxHashCode;
	hash_node_t*    pxNext;
	hash_node_t*    pxPrevious;
} hash_node_t;

typedef struct xHASH_T c_hash_t;
typedef struct xHASH_T 
{
    void *this;
    uint8_t(*add_str)(const c_hash_t*, char*, char* );
    uint8_t(*add_arr)(const c_hash_t*, char*, char*, uint16_t);
	
    uint8_t(*find)(const c_hash_t*, char*);
} c_hash_t;


/* Macros  ------------------------------------------------------------------*/
#ifndef USE_EXTERN_MACROS   //如果不使用外部宏

#endif

/* Private macro -------------------------------------------------------------*/
/* public interface ------------------------------------------------------------------*/
c_hash_t *hash_create(void);


#ifdef __cplusplus
}
#endif
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
