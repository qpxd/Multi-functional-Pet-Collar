/*****************************************************************************
* Copyright:
* File name: gy_sgp30.h
* Description: gy_sgp30气体传感器公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/30
*****************************************************************************/

#ifndef __GY_SGP30_H__
#define __GY_SGP30_H__


#define HUMIDITY_MAX               255.996f   /*最大的湿度补偿 g/m3*/
#define HUMIDITY_UN                   -1.0f   /*不需要湿度补偿*/
#define SGP30_START_MAX_TIME_S           50   /*SGP30启动时间*/

typedef struct __SGP30  c_sgp30;

typedef struct __SGP30
{
    void* this;

    /************************************************* 
    * Function: start 
    * Description: 启动颜色转换
    * Input : <this>           tcs34725对象
    * Output: 无
    * Return: <E_OK>     启动成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: sgp30初次上电有一个预热的过程，这个过程可能会长达二十几秒，后才会达到稳定
    *         在稳定后，还需要17s才能到到正常读数的状态。本函数将会过滤本状态。注：本函
    *         数的调用是阻塞的，时间在17~40s左右，具体看spg30的稳定速度。
    * Demo  :
    *         ret = sgp30.start(&sgp30);
    *         if(E_OK != ret)
    *         {
    *             log_error("sgp30 start failed.");
    *         }
    *************************************************/      
    int (*start)(const c_sgp30* this);
    
    /************************************************* 
    * Function: get 
    * Description: 获取当前co2浓度和tvoc（Total Volatile Organic Compounds”的英文缩写，意思是总挥发性有机化合物）浓度
    * Input : <this>  sgp30对象
    * Output: <humidity>  当前空气的绝对湿度，不需要进行湿度补偿时，填入HUMIDITY_UN  0~255.996fg/m3
    *         <co2>       当前空气co2浓度    400~60000ppm
    *         <tvoc>      当前控制tvoc浓度   0  ~60000ppb
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u16 co2 = 0,voc = 0;
    *
    *         ret = sgp30.get(&sgp30,HUMIDITY_UN,&co2,&voc);
    *         if(E_OK != ret )
    *         {
    *             log_error("Sgp30 get failed.");
    *         }
    *         log_inform("co2:%dppm,voc:%dppb",co2,voc);
    *************************************************/  
    int (*get)(const c_sgp30* this,float humidity,u16* co2,u16* voc);
}c_sgp30;


/************************************************* 
* Function: sgp30_creat 
* Description: 创建一个sgp30对象
* Input : <iic>  sgp30所在的iic总线
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_sgp30 sgp30 = {0}; 
*         
*	      sgp30 = sgp30_creat(&iic);
*         if(NULL == sgp30.this)
*         {
*             log_error("sgp30 creat failed.");
*         }
*************************************************/
c_sgp30 sgp30_creat(const c_my_iic* iic);

#endif


