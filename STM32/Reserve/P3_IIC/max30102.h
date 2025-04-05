/*****************************************************************************
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: max30102.h
* Description: max30102模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#ifndef MAX30102_H_
#define MAX30102_H_

typedef struct __C_MAX30102
{
    /************************************************* 
    * Function: init 
    * Description: 初始化max30102
    * Input : <cfg>      max30102所在iic总线
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :       
    *         ret = max30102.init(&iic);
    *         if(E_OK != ret )
    *         {
    *             log_error("max30102 init failed.");
    *         }
    *************************************************/      
    int (*init)(const c_my_iic* iic);       /*初始化*/
    
    /************************************************* 
    * Function: get 
    * Description: 获取心率血氧
    * Input : 无
    * Output: <heat>     当前心率
    *         <o2>       当前血氧
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u32 heat = 0;
    *         u32 o2   = 0;
    *         
    *         ret = max30102.get(&heat,&o2);
    *         if(E_OK != ret )
    *         {
    *             log_error("max30102 get failed.");
    *         }
    *         log_inform("heat:%d,o2:%d%%.",heat,o2);
    *************************************************/      
    int (*get)(u32* heat,u32* o2);   
}c_max30102;

extern const c_max30102 max30102;


#endif 
