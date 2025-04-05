/***************************************************************************** 
* Copyright: 2022-2024
* File name: mlx90614.h
* Description: 红外非接触测温传感器驱动模块公开头文件
* Author: xzh
* Version: v1.0
* Date: xxx 
*****************************************************************************/

#ifndef __MLX90614_H__
#define __MLX90614_H__

#define MLX90614_DEFAULT_ADDR   0x5A   /*出厂默认地址*/
#define MLX90614_UNIVERSAL_ADDR 0x00   /*通用地址 注：慎用 大部分iic器件 都会响应该地址 使用该*/
                                       /*使用该地址和mlx90614通讯后 之前设置的地址会被清除*/

typedef struct __MLX90614 c_mlx90614;

typedef struct __MLX90614
{
    void* this;

    /************************************************* 
    * Function: set_addr 
    * Description: 设置mlx90614的新地址
    * Input : <this>  mlx90614对象
    * Output: <addr>  新地址
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 该接口设置成功后 重启后生效
    * Demo  :
    *
    *         ret = mlx90614.set_addr(&mlx90614,0x55);
    *         if(E_OK != ret)
    *         {
    *             log_error("mlx90614 addr set failed.");
    *         }
    *************************************************/  
    int (*set_addr)(const c_mlx90614* this,u8 addr);
    
    /************************************************* 
    * Function: get 
    * Description: 获取当前环境温度和目标温度
    * Input : <this>  mlx90614对象
    * Output: <environment>  当前环境温度
    *         <target>       当前目标温度
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float temp = 0.0f,env = 0.0f;
    *
    *         ret = mlx90614.get(&mlx90614,&env,&temp);
    *         if(E_OK != ret)
    *         {
    *             log_error("mlx90614 get failed.");
    *         }
    *         log_inform("env:%.2f,temp:%.2f",env,temp);
    *************************************************/  
    int (*get)(const c_mlx90614* this,float* environment,float* target);
}c_mlx90614;

/************************************************* 
* Function: mlx90614_creat 
* Description: 创建一个mlx90614对象
* Input : <iic>  mlx90614所在的iic总线
*         <addr> mlx90614的器件地址
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_mlx90614 mlx90614 = {0}; 
*         
*	      mlx90614 = mlx90614_creat(&iic,MLX90614_DEFAULT_ADDR);
*         if(NULL == mlx90614.this)
*         {
*             log_error("mlx90614 creat failed.");
*         }
*************************************************/
c_mlx90614 mlx90614_creat(const c_my_iic* iic,u8 addr);

#endif

