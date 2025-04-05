/*****************************************************************************
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: mpu6050.c
* Description: mpu6050模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#include "driver_conf.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"



#ifdef DRIVER_MPU6050_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "mpu6050"  

//#define MPU_ACCEL_OFFS_REG		0X06	//accel_offs寄存器,可读取版本号,寄存器手册未提到
//#define MPU_PROD_ID_REG			0X0C	//prod id寄存器,在寄存器手册未提到
#define MPU_SELF_TESTX_REG		0X0D	//自检寄存器X
#define MPU_SELF_TESTY_REG		0X0E	//自检寄存器Y
#define MPU_SELF_TESTZ_REG		0X0F	//自检寄存器Z
#define MPU_SELF_TESTA_REG		0X10	//自检寄存器A
#define MPU_SAMPLE_RATE_REG		0X19	//采样频率分频器
#define MPU_CFG_REG				0X1A	//配置寄存器
#define MPU_GYRO_CFG_REG		0X1B	//陀螺仪配置寄存器
#define MPU_ACCEL_CFG_REG		0X1C	//加速度计配置寄存器
#define MPU_MOTION_DET_REG		0X1F	//运动检测阀值设置寄存器
#define MPU_FIFO_EN_REG			0X23	//FIFO使能寄存器
#define MPU_I2CMST_CTRL_REG		0X24	//IIC主机控制寄存器
#define MPU_I2CSLV0_ADDR_REG	0X25	//IIC从机0器件地址寄存器
#define MPU_I2CSLV0_REG			0X26	//IIC从机0数据地址寄存器
#define MPU_I2CSLV0_CTRL_REG	0X27	//IIC从机0控制寄存器
#define MPU_I2CSLV1_ADDR_REG	0X28	//IIC从机1器件地址寄存器
#define MPU_I2CSLV1_REG			0X29	//IIC从机1数据地址寄存器
#define MPU_I2CSLV1_CTRL_REG	0X2A	//IIC从机1控制寄存器
#define MPU_I2CSLV2_ADDR_REG	0X2B	//IIC从机2器件地址寄存器
#define MPU_I2CSLV2_REG			0X2C	//IIC从机2数据地址寄存器
#define MPU_I2CSLV2_CTRL_REG	0X2D	//IIC从机2控制寄存器
#define MPU_I2CSLV3_ADDR_REG	0X2E	//IIC从机3器件地址寄存器
#define MPU_I2CSLV3_REG			0X2F	//IIC从机3数据地址寄存器
#define MPU_I2CSLV3_CTRL_REG	0X30	//IIC从机3控制寄存器
#define MPU_I2CSLV4_ADDR_REG	0X31	//IIC从机4器件地址寄存器
#define MPU_I2CSLV4_REG			0X32	//IIC从机4数据地址寄存器
#define MPU_I2CSLV4_DO_REG		0X33	//IIC从机4写数据寄存器
#define MPU_I2CSLV4_CTRL_REG	0X34	//IIC从机4控制寄存器
#define MPU_I2CSLV4_DI_REG		0X35	//IIC从机4读数据寄存器

#define MPU_I2CMST_STA_REG		0X36	//IIC主机状态寄存器
#define MPU_INTBP_CFG_REG		0X37	//中断/旁路设置寄存器
#define MPU_INT_EN_REG			0X38	//中断使能寄存器
#define MPU_INT_STA_REG			0X3A	//中断状态寄存器

#define MPU_ACCEL_XOUTH_REG		0X3B	//加速度值,X轴高8位寄存器
#define MPU_ACCEL_XOUTL_REG		0X3C	//加速度值,X轴低8位寄存器
#define MPU_ACCEL_YOUTH_REG		0X3D	//加速度值,Y轴高8位寄存器
#define MPU_ACCEL_YOUTL_REG		0X3E	//加速度值,Y轴低8位寄存器
#define MPU_ACCEL_ZOUTH_REG		0X3F	//加速度值,Z轴高8位寄存器
#define MPU_ACCEL_ZOUTL_REG		0X40	//加速度值,Z轴低8位寄存器

#define MPU_TEMP_OUTH_REG		0X41	//温度值高八位寄存器
#define MPU_TEMP_OUTL_REG		0X42	//温度值低8位寄存器

#define MPU_GYRO_XOUTH_REG		0X43	//陀螺仪值,X轴高8位寄存器
#define MPU_GYRO_XOUTL_REG		0X44	//陀螺仪值,X轴低8位寄存器
#define MPU_GYRO_YOUTH_REG		0X45	//陀螺仪值,Y轴高8位寄存器
#define MPU_GYRO_YOUTL_REG		0X46	//陀螺仪值,Y轴低8位寄存器
#define MPU_GYRO_ZOUTH_REG		0X47	//陀螺仪值,Z轴高8位寄存器
#define MPU_GYRO_ZOUTL_REG		0X48	//陀螺仪值,Z轴低8位寄存器

#define MPU_I2CSLV0_DO_REG		0X63	//IIC从机0数据寄存器
#define MPU_I2CSLV1_DO_REG		0X64	//IIC从机1数据寄存器
#define MPU_I2CSLV2_DO_REG		0X65	//IIC从机2数据寄存器
#define MPU_I2CSLV3_DO_REG		0X66	//IIC从机3数据寄存器

#define MPU_I2CMST_DELAY_REG	0X67	//IIC主机延时管理寄存器
#define MPU_SIGPATH_RST_REG		0X68	//信号通道复位寄存器
#define MPU_MDETECT_CTRL_REG	0X69	//运动检测控制寄存器
#define MPU_USER_CTRL_REG		0X6A	//用户控制寄存器
#define MPU_PWR_MGMT1_REG		0X6B	//电源管理寄存器1
#define MPU_PWR_MGMT2_REG		0X6C	//电源管理寄存器2 
#define MPU_FIFO_CNTH_REG		0X72	//FIFO计数寄存器高八位
#define MPU_FIFO_CNTL_REG		0X73	//FIFO计数寄存器低八位
#define MPU_FIFO_RW_REG			0X74	//FIFO读写寄存器
#define MPU_DEVICE_ID_REG		0X75	//器件ID寄存器

#define IIC_SPEED  0

static int MPU_Init(const c_my_iic* iic);
static u8 MPU_Set_Rate(u16 rate);
static void m_task(void* pdata);
static int m_get(float* x,float* y,float* z);
static int m_get_g(short* x,short* y,short* z);   
static int m_get_a(short* x,short* y,short* z);  

static TaskHandle_t   g_task  ;  /*任务句柄*/
static c_my_iic       g_iic;
static float pitch,roll,yaw;
static short g_x,g_y,g_z;
static short a_x,a_y,a_z;

const c_mpu6050 mpu6050 = {MPU_Init,m_get,m_get_g,m_get_a};

//初始化MPU6050
//返回值:0,成功
//    其他,错误代码
int MPU_Init(const c_my_iic* iic)
{ 
	u8 res;
    int ret = 0;
    u8 data = 0;
    BaseType_t  os_ret = pdFALSE;
	
    /*检查参数*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    } 
    memcpy((void*)&g_iic,iic,sizeof(c_my_iic));  /*保存iic*/
    
	/*复位MPU6050*/
    data = 0X80;
    ret = MPU_Write_Len(MPU_ADDR,MPU_PWR_MGMT1_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    vTaskDelay(100);
    
    /*唤醒MPU6050*/
    data = 0X00;
    ret = MPU_Write_Len(MPU_ADDR,MPU_PWR_MGMT1_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    /*设置陀螺仪满量程范围 陀螺仪传感器,±2000dps */
    //fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
    data = 3 << 3;
    ret = MPU_Write_Len(MPU_ADDR,MPU_GYRO_CFG_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    /*设置MPU6050加速度传感器满量程范围*/
    //fsr:0,±2g;1,±4g;2,±8g;3,±16g
    data = 0 << 3;
    ret = MPU_Write_Len(MPU_ADDR,MPU_ACCEL_CFG_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }

    /*设置数字低通滤波器  设置采样率50Hz*/
    data = 3;
    ret = MPU_Write_Len(MPU_ADDR,MPU_CFG_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }

    /*关闭所有中断*/
    data = 0;
    ret = MPU_Write_Len(MPU_ADDR,MPU_INT_EN_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    /*I2C主模式关闭*/
    data = 0;
    ret = MPU_Write_Len(MPU_ADDR,MPU_USER_CTRL_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    /*关闭FIFO*/
    data = 0;
    ret = MPU_Write_Len(MPU_ADDR,MPU_FIFO_EN_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    /*INT引脚低电平有效*/
    data = 0;
    ret = MPU_Write_Len(MPU_ADDR,MPU_INTBP_CFG_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }    
    
    /*器件ID检查*/
    data = 0;
    ret = MPU_Read_Len(MPU_ADDR,MPU_DEVICE_ID_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }   
    if(MPU_ADDR != data)
    {
        log_error("ID error");
        return E_ERROR;
    }
        
    /*设置CLKSEL,PLL X轴为参考*/
    data = 0x01;
    ret = MPU_Write_Len(MPU_ADDR,MPU_PWR_MGMT1_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    } 
    
    /*加速度与陀螺仪都工作*/
    data = 0;
    ret = MPU_Write_Len(MPU_ADDR,MPU_PWR_MGMT2_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }     

    /*设置数字低通滤波器  设置采样率50Hz*/
    data = 3;
    ret = MPU_Write_Len(MPU_ADDR,MPU_CFG_REG,1,&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }    
    
    /*创建任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task          ,
                         (const char*    )"mpu6050 task" ,
                         (uint16_t       )MPU6050_TASK_STK  ,
                         (void*          )NULL          ,
                         (UBaseType_t    )MPU6050_TASK_PRO  ,
                         (TaskHandle_t*  )&g_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed");
        return E_ERROR;
    }

	return E_OK;
}



//设置MPU6050的数字低通滤波器
//lpf:数字低通滤波频率(Hz)
//返回值:0,设置成功
//    其他,设置失败 
u8 MPU_Set_LPF(u16 lpf)
{
	u8 data=0;
	if(lpf>=188)data=1;
	else if(lpf>=98)data=2;
	else if(lpf>=42)data=3;
	else if(lpf>=20)data=4;
	else if(lpf>=10)data=5;
	else data=6; 
	return MPU_Write_Len(MPU_ADDR,MPU_CFG_REG,1,&data);//设置数字低通滤波器  
}
//设置MPU6050的采样率(假定Fs=1KHz)
//rate:4~1000(Hz)
//返回值:0,设置成功
//    其他,设置失败 
u8 MPU_Set_Rate(u16 rate)
{
	u8 data;
	if(rate>1000)rate=1000;
	if(rate<4)rate=4;
	data=1000/rate-1;
	data=MPU_Write_Len(MPU_ADDR,MPU_SAMPLE_RATE_REG,1,&data);	//设置数字低通滤波器
 	return MPU_Set_LPF(rate/2);	//自动设置LPF为采样率的一半
}

//得到温度值
//返回值:温度值(扩大了100倍)
short MPU_Get_Temperature(void)
{
    u8 buf[2]; 
    short raw;
	float temp;
	MPU_Read_Len(MPU_ADDR,MPU_TEMP_OUTH_REG,2,buf); 
    raw=((u16)buf[0]<<8)|buf[1];  
    temp=36.53+((double)raw)/340;  
    return temp*100;;
}
//得到陀螺仪值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
u8 MPU_Get_Gyroscope(short *gx,short *gy,short *gz)
{
    u8 buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
	if(res==0)
	{
		*gx=((u16)buf[0]<<8)|buf[1];  
		*gy=((u16)buf[2]<<8)|buf[3];  
		*gz=((u16)buf[4]<<8)|buf[5];
	} 	
    return res;;
}
//得到加速度值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
u8 MPU_Get_Accelerometer(short *ax,short *ay,short *az)
{
    u8 buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
	if(res==0)
	{
		*ax=((u16)buf[0]<<8)|buf[1];  
		*ay=((u16)buf[2]<<8)|buf[3];  
		*az=((u16)buf[4]<<8)|buf[5];
	} 	
    return res;;
}
//IIC连续写
//addr:器件地址 
//reg:寄存器地址
//len:写入长度
//buf:数据区
//返回值:0,正常
//    其他,错误代码
u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
    iic_cmd_handle_t  iic_cmd = NULL;
    int ret = 0;
    u8 w_addr = 0;

    /*检查参数*/
    if(NULL == g_iic.this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }
    
    /* 发起I2C总线启动信号 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }    
    
    //发送器件地址+写命令
    w_addr = (addr<<1)|0;
    ret = iic_write_bytes(iic_cmd,&w_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    //写寄存器地址
    ret = iic_write_bytes(iic_cmd,&reg,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }    
    
    /*写入 N个字节*/
    ret = iic_write_bytes(iic_cmd,buf,len);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }   

    /* 发送I2C总线停止信号 */
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动iic 序列*/
    g_iic.clear(&g_iic,w_addr,I2C_ACK_WAIT_DEFAULT_TIME,IIC_SPEED);
    ret = g_iic.begin(&g_iic,iic_cmd,IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    return E_OK;
    
    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR;     
} 
//IIC连续读
//addr:器件地址
//reg:要读取的寄存器地址
//len:要读取的长度
//buf:读取到的数据存储区
//返回值:0,正常
//    其他,错误代码
u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{ 
    iic_cmd_handle_t  iic_cmd = NULL;
    int ret = 0;
    u8 w_addr = 0,r_addr = 0;

    /*检查参数*/
    if(NULL == g_iic.this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }
    
    /* 发起I2C总线启动信号 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }    
    
    //发送器件地址+写命令
    w_addr = (addr<<1)|0;
    ret = iic_write_bytes(iic_cmd,&w_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    //写寄存器地址
    ret = iic_write_bytes(iic_cmd,&reg,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /* 发起I2C总线启动信号 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }  
    
    /*写寄存器地址*/
    r_addr = (addr<<1)|1;
    ret = iic_write_bytes(iic_cmd,&r_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*读取 N个字节*/
    ret = iic_read_bytes(iic_cmd,buf,len);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }   

    /* 发送I2C总线停止信号 */
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动iic 序列*/
    ret = g_iic.begin(&g_iic,iic_cmd,IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    return E_OK;
    
    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

#define PI (3.1416)
void MPU_Calculate(float *x, float *y, float *z)
{
    float f_g[3];
	int16_t a_x, a_y, a_z;
	MPU_Get_Accelerometer(&a_x, &a_y, &a_z);
    f_g[0] = (float)(a_x / 16384.0);
    f_g[1] = (float)(a_y / 16384.0);
    f_g[2] = (float)(a_z / 16384.0);

    *x    = atan(f_g[0]/sqrt(f_g[1]*f_g[1] + f_g[2]*f_g[2])) * 180 / PI;
    *y   = atan(f_g[1]/sqrt(f_g[0]*f_g[0] + f_g[2]*f_g[2])) * 180 / PI;
    *z     = atan(f_g[2]/sqrt(f_g[0]*f_g[0] + f_g[1]*f_g[1])) * 180 / PI;
    
//    *x    *= 1.1;
//    *y    *= 1.1;
//    *z    *= 1.1;
}

static int m_get(float* x,float* y,float* z)
{
    /*参数检查*/
    if(NULL == x || NULL == y || NULL == z)
    {
        log_error("Param error.");
        return E_NULL;
    }
    
    *x = roll  ;
    *y = pitch ; 
    *z = yaw   ;
    
    return E_OK;
}

static int m_get_g(short* x,short* y,short* z)
{
    /*参数检查*/
    if(NULL == x || NULL == y || NULL == z)
    {
        log_error("Param error.");
        return E_NULL;
    }
    
    *x = g_x  ;
    *y = g_y  ; 
    *z = g_z  ;
    
    return E_OK;    
}    
static int m_get_a(short* x,short* y,short* z)
{
    /*参数检查*/
    if(NULL == x || NULL == y || NULL == z)
    {
        log_error("Param error.");
        return E_NULL;
    }
    
    *x = a_x  ;
    *y = a_y  ; 
    *z = a_z  ;
    
    return E_OK;  
}

static void m_task(void* pdata)
{
    u8 i = 5;
    
//    /*校准*/
//    while(mpu_dmp_init())
// 	{
//        
// 		vTaskDelay(200);
//        
//        if(i >= 5)
//        {
//            i = 5;
//            log_inform("mpu6050 is zeroing.");
//        }
//        else
//        {
//            --i;
//        }
//	}  
    
    while(1)
    {
//        if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
//        {
//            MPU_Get_Gyroscope(&g_x,&g_y,&g_z);
//            MPU_Get_Accelerometer(&a_x,&a_y,&a_z);
//            //log_inform("x:%.2f,y:%.2f,z:%.2f",pitch,roll,yaw);
//        }
        vTaskDelay(100);
    }
}
#endif