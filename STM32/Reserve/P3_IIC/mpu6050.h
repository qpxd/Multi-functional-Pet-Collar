/*****************************************************************************
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: mpu6050.h
* Description: mpu6050模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#ifndef __MPU6050_H
#define __MPU6050_H

//如果AD0脚(9脚)接地,IIC地址为0X68(不包含最低位).
//如果接V3.3,则IIC地址为0X69(不包含最低位).
#define MPU_ADDR				0X68

typedef struct __C_MPU6050
{
    /************************************************* 
    * Function: init 
    * Description: 初始化mpu6050
    * Input : <cfg>      mpu6050所在iic总线
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :       
    *         ret = mpu6050.init(&iic);
    *         if(E_OK != ret )
    *         {
    *             log_error("mpu6050 init failed.");
    *         }
    *************************************************/       
    int (*init)(const c_my_iic* iic);         /*初始化*/  

    /************************************************* 
    * Function: get 
    * Description: 获取当前欧拉角
    * Input : 无
    * Output: <x>        俯仰角
    *         <y>        横滚角
    *         <z>        方向角
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float x = 0;
    *         float y = 0;
    *         float z = 0;
    *         
    *         ret = mpu6050.get(&x,&y,&z);
    *         if(E_OK != ret )
    *         {
    *             log_error("mpu6050 get failed.");
    *         }
    *         log_inform("x:%.2f,y:%.2f,z:%.2f",x,y,z);
    *************************************************/        
    int (*get)(float* x,float* y,float* z);   /*获取方位角*/
    
    int (*get_g)(short* x,short* y,short* z);   
    int (*get_a)(short* x,short* y,short* z);   
}c_mpu6050;

extern const c_mpu6050 mpu6050;




u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf);//IIC连续写
u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf); //IIC连续读 

short MPU_Get_Temperature(void);

u8 MPU_Get_Gyroscope(short *gx,short *gy,short *gz);
u8 MPU_Get_Accelerometer(short *ax,short *ay,short *az);
void MPU_Calculate(float *x, float *y, float *z);
#endif
/*初始化iic*/
//iic = iic_creat(GPIOB,GPIO_PIN_7,GPIOB,GPIO_PIN_6);
//if(NULL == iic.this)
//{
//    log_error("iic creat failed.");
//}

/*初始化mpu6050*/
//ret = mpu6050.init(&iic);
//if(E_OK != ret )
//{
//    log_error("mpu6050 init failed.");
//}

/*获取当前欧拉角*/
//ret = mpu6050.get(&x,&y,&z);
//if(E_OK != ret )
//{
//    log_error("mpu6050 get failed.");
//}
//log_inform("x:%.2f,y:%.2f,z:%.2f",x,y,z);





































