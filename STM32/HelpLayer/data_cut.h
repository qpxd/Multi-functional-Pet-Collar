/*****************************************************************************
* Copyright:
* File name: data_cut.h
* Description: 数据裁剪模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/02/23
* log:  V1.0  2022/02/23
*       发布：
*****************************************************************************/

#ifndef __DATA_CUT_H__
#define __DATA_CUT_H__

typedef struct _C_DATA_CUT c_data_cut;

#define SHOW_CUT_DATA 0

#define CUT_DATA_ADD_MAX_LEN   512

#define CUT_TYPE_FB  0  /*头尾裁剪*/
#define CUT_TYPE_B   1  /*尾部裁剪*/

typedef struct DATA_CUT_CFG
{
    const u8* o_head     ;  /*头部*/
    u16       o_head_len ;  /*头部长度*/
    const u8* o_end      ;  /*尾部*/
    u16       o_end_len  ;  /*尾部长度*/
    u16       o_max_len  ;  /*含头含尾*/
}data_cut_cfg;

typedef struct _C_DATA_CUT
{
    void* this;

    //int (*cut_fb)(const c_data_cut* this,const u8* data,u16 data_len);
    //int (*cut_b) (const c_data_cut* this,const u8* data,u16 data_len);
    
    int (*close) (const c_data_cut* this);
    int (*open)  (const c_data_cut* this);
    int (*set_custom_cut)(const c_data_cut* this,int (*cust_cut)  (const u8* buf,u16 buf_len,u8** head,u8** end));
    
    int (*clear) (const c_data_cut* this);
    int (*add)   (const c_data_cut* this,const u8* data,u16 data_len);
    int (*get)   (const c_data_cut* this,const u8** data,u16* data_len,TickType_t xTicksToWait);
    int (*back)  (const c_data_cut* this,      u8* data);
    int (*get_static)(const c_data_cut* this,u8* data,u16* data_len,TickType_t xTicksToWait);
}c_data_cut;

c_data_cut data_cut_creat(u8 cut_type,const data_cut_cfg* cfg);


#endif

